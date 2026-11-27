# The web browser

The easiest way in: nothing to buy or burn, and nothing on the console is erased. The PS5's web
browser opens an **exploit host**, a web page that runs the exploit, then loads a HEN.

| Your firmware | The exploit | Its project |
|---|---|---|
| 1.00 – 5.50 | **UMTX** | [PSFree and Lapse](https://github.com/EchoStretch/PSFree/blob/main/README.md) |
| 7.00 – 13.60 | **Relapse** | [Relapse, on GBAtemp](https://gbatemp.net/threads/relapse-ps5-kernel-exploit-brings-homebrew-to-firmware-7-00-13-60.684829/) |

From 5.51 to 6.99 the browser has no way in: use [a Blu-ray disc](blu-ray.md) or
[the YouTube app](youtube.md). Not sure which fits? [Pick your firmware](get-started.md#pick).

> [!IMPORTANT]
> Read the whole page before you start, then follow it in order. This guide doesn't host exploits: for
> the exploit host itself, its project's page has the last word.

## What you need {#what-you-need}

- **A PS5 on a firmware in the table**, any model. Relapse runs on the Slim and the Pro too.
- **The PS5 on your network**, by Wi-Fi or a cable, with an internet connection.
- **An exploit host for your exploit**, from its project's page above. Use one the project links to:
  a page that asks for money, an account or your PSN sign-in isn't one.
- **A computer or a phone on the same network**, for later: it copies Playtendoh to the PS5.

## Section I: Prep work {#prep}

1. Check your firmware once more: **Settings > System > System Software > Console Information**. It
   must be in the table above.
2. Turn off automatic updates, if you haven't: **Settings > System > System Software > System Software
   Update and Settings**. Turn off both downloading and installing update files automatically.
3. Close every game and app, then **Restart** the PS5 from the power menu (not rest mode). An exploit
   works best on a console that has just started.

## Section II: Open the exploit host {#open-host}

The PS5 has no browser on its home screen. An exploit host is opened through a page the console does
open: most hosts use the **User's Guide** (**Settings > User's Guide, Health and Safety, and Other
Information > User's Guide**), with the DNS server address the host gives set in the PS5's network
settings. Each host says which way it uses.

1. Set up the way into the browser that your host's page gives, exactly as it says.
2. Open it. The exploit host's page loads in the PS5's browser.

## Section III: Run the exploit {#run}

Run the one for your firmware.

### UMTX, 1.00 to 5.50 {#umtx}

1. On the exploit host's page, start the jailbreak.
2. Don't press anything until the page says it's done, or a notification says the HEN has loaded.

### Relapse, 7.00 to 13.60 {#relapse}

1. On the exploit host's page, start Relapse.
2. Don't press anything until the page says it's done, or a notification says the HEN has loaded.

> [!TIP]
> **A kernel exploit doesn't always work the first time.** If the page stops, the PS5 freezes or it
> restarts, nothing is harmed: restart the PS5, open the host again and run it again. It can take a few
> tries.

## Section IV: Load the HEN {#hen}

Most exploit hosts load a HEN once the exploit has run, or have you pick one. Pick the one for your
firmware:

- **Up to 12.70: etaHEN.** It's the HEN Playtendoh is tested with most.
- **Above 12.70, to 13.60: Kstuff Lite**, with an ELF loader on port 9021.

Wait for the HEN's notification before you go on. The next page has each HEN, and the one setting
Playtendoh needs in it.

## After every restart {#again}

The jailbreak doesn't survive turning the PS5 off. After every full power-off: open the exploit host
again, run the exploit, load your HEN, then start Playtendoh. Rest mode keeps the jailbreak, so you can
usually put the PS5 in rest mode instead.
