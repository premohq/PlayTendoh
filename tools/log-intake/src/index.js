// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh's report intake, a Cloudflare Worker (README.md):
//   POST /v1/report               one report from the app (application/json, 64 KB at most): checked,
//                                 rate-limited, scrubbed again and kept in D1. 201, or 400 (dropped by the
//                                 app), 413, 415 or 429 (kept by the app and sent later).
//   GET  /v1/compatibility.json   the table GitHub Pages shows: one row a game, no install ID, no log.
//   GET  /v1/reports              the reports themselves, logs and all, for the owner's triage
//                                 (tools/triage/fetch_reports.py), with the READ_TOKEN secret alone: none
//                                 set, it answers 404 as if it were not there. No install ID: a console.
// Nothing about the sender is kept: its network address is only the rate limiter's key.

import { MAX_BYTES, validate, compatibility } from './report.js';

const json = (body, status = 200, headers = {}) =>
	new Response(JSON.stringify(body), { status, headers: { 'content-type': 'application/json', ...headers } });

async function take(request, env) {
	if (!(request.headers.get('content-type') || '').toLowerCase().startsWith('application/json')) return json({ error: 'json only' }, 415);
	const declared = Number(request.headers.get('content-length'));
	if (declared > MAX_BYTES) return json({ error: 'size' }, 413);
	const address = request.headers.get('cf-connecting-ip') || 'unknown';
	if (env.ADDRESS_LIMITER && !(await env.ADDRESS_LIMITER.limit({ key: address })).success) return json({ error: 'later' }, 429);
	const body = await request.text();
	if (!body || body.length > MAX_BYTES) return json({ error: 'size' }, 413);
	let report;
	try {
		report = JSON.parse(body);
	} catch {
		return json({ error: 'not json' }, 400);
	}
	const { row, error } = validate(report);
	if (error) return json({ error }, 400);
	if (env.INSTALL_LIMITER && !(await env.INSTALL_LIMITER.limit({ key: row.install })).success) return json({ error: 'later' }, 429);
	const columns = Object.keys(row);
	await env.DB.prepare(`INSERT INTO reports (received, ${columns.join(', ')}) VALUES (?, ${columns.map(() => '?').join(', ')})`)
		.bind(Math.floor(Date.now() / 1000), ...columns.map((c) => row[c]))
		.run();
	return json({ ok: true }, 201);
}

async function table(env, ctx, request) {
	const cache = caches.default;
	const hit = await cache.match(request);
	if (hit) return hit;
	const { results } = await env.DB.prepare(
		'SELECT received, install, app, side, title_id, name, result, error, seconds, firmware, model FROM reports'
	).all();
	const response = json(compatibility(results, Math.floor(Date.now() / 1000)), 200, {
		'cache-control': 'public, max-age=3600',
		'access-control-allow-origin': '*',
	});
	ctx.waitUntil(cache.put(request, response.clone()));
	return response;
}

// -- GET /v1/reports ---------------------------------------------------------------------------------
// Filters (each optional): after (a report's id: those after it, oldest first, for what came since the
// last fetch), before (those before it, newest first, the page after the newest; neither: the newest
// first), since (seconds since 1970), side, emulator, title (a title
// ID), app (a version), commit (its first characters), result (ended, failed, crashed, closed, a comma
// between), limit (1 to READ_MOST, 50 by default) and logs=0 (the reports without their logs).
export const READ_MOST = 200;
const RESULTS = ['ended', 'failed', 'crashed', 'closed'];
const NAMED = /^[a-z0-9][a-z0-9-]{0,23}$/;

export function readQuery(params) {
	const where = [], values = [];
	const number = (name) => {
		const text = params.get(name);
		if (text === null) return null;
		return /^\d{1,12}$/.test(text) ? Number(text) : NaN;
	};
	const after = number('after'), before = number('before'), since = number('since'), limit = number('limit');
	if ([after, before, since, limit].some(Number.isNaN)) return { error: 'after, before, since and limit are whole numbers' };
	if (after !== null) where.push('id > ?'), values.push(after);
	if (before !== null) where.push('id < ?'), values.push(before);
	if (since !== null) where.push('received >= ?'), values.push(since);
	for (const [name, column, shape] of [['side', 'side', NAMED], ['emulator', 'emulator', NAMED], ['title', 'title_id', /^[0-9a-f]{16}$/],
		['app', 'app', /^[0-9][0-9a-z.-]{0,23}$/]]) {
		const value = params.get(name);
		if (value === null) continue;
		if (!shape.test(value)) return { error: `a bad ${name}` };
		where.push(`${column} = ?`), values.push(value);
	}
	const commit = params.get('commit');
	if (commit !== null) {
		if (!/^[0-9a-f]{4,40}$/.test(commit)) return { error: 'a bad commit' };
		where.push('commit_id LIKE ?'), values.push(`${commit}%`);
	}
	const results = params.get('result');
	if (results !== null) {
		const asked = results.split(',');
		if (!asked.length || asked.some((r) => !RESULTS.includes(r))) return { error: `result: ${RESULTS.join(', ')}` };
		where.push(`result IN (${asked.map(() => '?').join(', ')})`), values.push(...asked);
	}
	const logs = params.get('logs') !== '0';
	const ascending = after !== null;
	values.push(Math.min(Math.max(limit ?? 50, 1), READ_MOST));
	const columns = 'id, received, install, app, commit_id, side, emulator, title_id, name, result, error, seconds, firmware, model, jit, hen, settings' +
		(logs ? ', log, emulator_logs' : '');
	return {
		sql: `SELECT ${columns} FROM reports${where.length ? ` WHERE ${where.join(' AND ')}` : ''} ORDER BY id ${ascending ? 'ASC' : 'DESC'} LIMIT ?`,
		values, logs,
	};
}

const hex = (bytes) => [...new Uint8Array(bytes)].map((b) => b.toString(16).padStart(2, '0')).join('');
const digest = async (text) => new Uint8Array(await crypto.subtle.digest('SHA-256', new TextEncoder().encode(text)));

// the token, compared by digest, so the time taken says nothing of it
async function authorized(request, token) {
	const given = (request.headers.get('authorization') || '').replace(/^Bearer\s+/i, '');
	if (!given) return false;
	const [a, b] = await Promise.all([digest(token), digest(given)]);
	let differ = 0;
	for (let i = 0; i < a.length; i++) differ |= a[i] ^ b[i];
	return differ === 0;
}

// an install ID as a console: a hash of it keyed by the token, the same for every report of one console,
// to count them, and never the ID (docs/CRASH-TRIAGE.md, principle 6)
const consoleOf = async (token, install) => hex(await digest(`${token}:${install}`)).slice(0, 12);

const parsed = (text, otherwise) => {
	try {
		return text ? JSON.parse(text) : otherwise;
	} catch {
		return otherwise;
	}
};

async function read(request, env) {
	const token = env.READ_TOKEN || '';
	if (token.length < 32) return json({ error: 'not found' }, 404);
	const address = request.headers.get('cf-connecting-ip') || 'unknown';
	if (env.ADDRESS_LIMITER && !(await env.ADDRESS_LIMITER.limit({ key: address })).success) return json({ error: 'later' }, 429);
	if (!(await authorized(request, token))) return json({ error: 'token' }, 401, { 'www-authenticate': 'Bearer' });
	const query = readQuery(new URL(request.url).searchParams);
	if (query.error) return json({ error: query.error }, 400);
	const { results } = await env.DB.prepare(query.sql).bind(...query.values).all();
	const reports = [];
	for (const row of results) {
		const report = {
			id: row.id, received: row.received, console: await consoleOf(token, row.install), app: row.app, commit: row.commit_id,
			side: row.side, emulator: row.emulator, titleId: row.title_id, name: row.name, result: row.result, error: row.error ?? null,
			seconds: row.seconds ?? null, firmware: row.firmware, model: row.model, jit: !!row.jit, hen: !!row.hen, settings: parsed(row.settings, {}),
		};
		if (query.logs) {
			report.log = row.log ?? null;
			report.logs = parsed(row.emulator_logs, []);
		}
		reports.push(report);
	}
	// next: the id to ask after (or before) next time, the last one served (none: the one asked with); more: a
	// full page, so there may be more
	const asked = new URL(request.url).searchParams, from = asked.get('after') ?? asked.get('before');
	const next = reports.length ? reports[reports.length - 1].id : from === null ? null : Number(from);
	return json({ reports, next, more: reports.length === query.values[query.values.length - 1] }, 200, { 'cache-control': 'no-store' });
}

const DAILY = '17 3 * * *';

// The Pages workflow started on the default branch, with PAGES_TOKEN (a fine-grained token for this repository
// alone, Actions: read and write; `npx wrangler secret put PAGES_TOKEN`). None: nothing is started.
export async function rebuildSite(env, fetcher = fetch) {
	if (!env.PAGES_TOKEN) return false;
	const repository = env.PAGES_REPOSITORY || 'premohq/PlayTendoh';
	const response = await fetcher(`https://api.github.com/repos/${repository}/actions/workflows/pages.yml/dispatches`, {
		method: 'POST',
		headers: {
			authorization: `Bearer ${env.PAGES_TOKEN}`,
			accept: 'application/vnd.github+json',
			'user-agent': 'playtendoh-reports',
			'x-github-api-version': '2022-11-28',
		},
		body: JSON.stringify({ ref: env.PAGES_BRANCH || 'main' }),
	});
	if (response.status !== 204) console.log('pages dispatch', response.status);
	return response.status === 204;
}

export default {
	async fetch(request, env, ctx) {
		const { pathname } = new URL(request.url);
		try {
			if (pathname === '/v1/report' && request.method === 'POST') return await take(request, env);
			if (pathname === '/v1/compatibility.json' && request.method === 'GET') return await table(env, ctx, request);
			if (pathname === '/v1/reports' && request.method === 'GET') return await read(request, env);
			return json({ error: 'not found' }, 404);
		} catch (e) {
			console.log('intake error', String((e && e.message) || e).slice(0, 200));
			return json({ error: 'intake error' }, 500);
		}
	},
	// once a day (wrangler.toml's first cron): reports over 180 days old go. Every hour (its second): the site
	// built again, as GitHub runs the Pages workflow's own hourly schedule only every few hours
	async scheduled(event, env, ctx) {
		if (event.cron === DAILY) {
			await env.DB.prepare('DELETE FROM reports WHERE received < ?').bind(Math.floor(Date.now() / 1000) - 180 * 86400).run();
			return;
		}
		await rebuildSite(env);
	},
};
