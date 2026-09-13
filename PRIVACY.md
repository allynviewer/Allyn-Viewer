# Allyn Viewer Privacy Policy

Allyn Viewer is a third-party Second Life client. This page describes what data the viewer sends, stores, or uses. This software is not provided or supported by Linden Lab.

## Second Life grid

When you log in, the viewer connects to the Second Life grid you choose and sends the credentials and session data that grid requires (login name, password, and the usual viewer protocol). That traffic is governed by the [Linden Lab privacy policy](https://lindenlab.com/privacy). Allyn Viewer does not send your password to any other server.

## Presence count

While you are logged into Second Life, the viewer sends a periodic heartbeat to `https://allynviewer.discloud.app/` so the project site can show how many Allyn users are online. The payload is: avatar UUID, viewer version, and action (`heartbeat` / `logout`). Passwords are not included. This runs automatically after login to Second Life; it is not sent on other grids.

## Optional AI translation

AI translation is off by default. If you enable it and enter your own API key, the chat or IM text you choose to translate is sent to the provider you configure (OpenAI, Anthropic, OpenRouter, OpenCode Zen, or a custom endpoint). Allyn Viewer does not supply an API key and does not operate that service. That provider’s privacy policy applies to the text you send.

## Local data

Settings, cache, chat logs, and crash logs stay on your computer (including `%APPDATA%\AllynViewer\`). Crash logs are written only if the viewer crashes. They leave your machine only if you attach them to a GitHub issue or otherwise send them yourself.

## What we do not collect

We do not require personal data to install or uninstall the viewer. We do not sell user data. We do not send usernames or passwords anywhere except the grid login you use.

## Contact

https://github.com/allynviewer/Allyn-Viewer/issues
