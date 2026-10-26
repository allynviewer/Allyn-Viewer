# Allyn Viewer Privacy Policy

Allyn Viewer is a third-party Second Life client. This page describes what data the viewer sends, stores, or uses. This software is not provided or supported by Linden Lab, the makers of Second Life.

## Second Life grid

When you log in, the viewer connects to the Second Life grid you choose and sends the credentials and session data that grid requires (login name, password, and the usual viewer protocol). That traffic is governed by the [Linden Lab privacy policy](https://lindenlab.com/privacy). Allyn Viewer does not send your password to any other server.

## Presence count

While you are logged into Second Life, the viewer sends a heartbeat about once a minute, and a logout when that session ends, to `https://allynviewer.discloud.app/api/presenca`. The payload is the avatar UUID, the viewer version, and the action (`heartbeat` or `logout`). Passwords are not included. This runs automatically after login to Second Life. It is not sent on other grids, and the viewer has no preference to turn it off.

The viewer also reads `https://allynviewer.discloud.app/api/status`. That response is two numbers: how many people are online in Second Life, and how many are using Allyn Viewer. The login screen shows those counts. The avatar UUID is not part of that response and is not shown to other users.

## Optional AI translation

AI translation is off by default. If you enable it and enter your own API key, the chat or IM text you choose to translate is sent to the provider you configure (OpenAI, Anthropic, OpenRouter, OpenCode Zen, or a custom endpoint). Allyn Viewer does not supply an API key and does not operate that service. That provider’s privacy policy applies to the text you send.

## Local data

Settings, cache, and chat logs stay on your computer (including `%APPDATA%\AllynViewer\`).

Crash reports default to Always send (Preferences → General → Crash reports). After a crash, the next launch can post a report to `https://allynviewer.discloud.app/api/crash` when that setting is Always send. The report contains the viewer channel, version, and build, the crash event, how long the session ran, the operating system, CPU, RAM, graphics card, OpenGL version, a short reason, and an excerpt of the session log. Before sending, the viewer drops lines that look like passwords, sessions, or login, and it redacts e-mail addresses, IPv4 addresses, and the local account folder name. Choose Never send to stop this. You can still attach a log to a GitHub issue yourself.

## What we do not collect

We do not require personal data to install or uninstall the viewer. We do not sell user data. We do not send usernames or passwords anywhere except the grid login you use. We do not publish avatar UUIDs. The public presence display is the online count only.

## Contact

Linden Lab does not support this program. Allyn Viewer support is through GitHub (https://github.com/allynviewer/Allyn-Viewer/issues), Discord (https://discord.gg/y6uaVeD79m), and e-mail (allynviewer@gmail.com).
