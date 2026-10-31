# Desktop online services

OpenEgg uses a replacement server. It does not sign in to PSN or provide PSN
services. Users can create replacement-server accounts directly in OpenEgg, or
use a PSN/RPCN ticket when available.
Only a verified PS3/Vita PSN ticket can create or renew the long-lived saved
ticket session. Local integration fixtures do not verify a user-provided
ticket or prove ownership of a real PSN identity.

| Service | Current desktop behavior |
| --- | --- |
| Account identity | Native OpenEgg account registration/sign-in and retail ticket sign-in are supported. OpenEgg registration does not verify email yet; PSN sign-in remains unavailable. |
| Watermelon community API | Client routes and replacement-server endpoints cover levels, search, metadata, ratings, follows, queues, albums, leaderboards, activity, cloud saves, and reports. Retail parity is still being tested. |
| Friends, featured content, news, EULA, translations | Loaded from the replacement server where configured. Results depend on server data. |
| Trophies, PlayStation Store, web browser, Vita Near, platform SNS | No desktop backend. Calls are registered but unavailable or inert. |
| PSN account restrictions and chat permissions | Desktop INI fallbacks; these are not queried from PSN. |

Run the probes under `tests/networking/` against a local server for client
checks. They do not establish retail parity or validate a genuine PSN ticket.
