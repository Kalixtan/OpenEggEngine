# Desktop online services

OpenEgg uses a replacement server. It does not sign in to PSN, provide PSN
services, or verify that an Online ID belongs to the person using it unless a
server accepts and verifies a PSN ticket. Ticket login is the intended account
setup; saved server sessions are kept in `OpenEgg.ini`.

| Service | Current desktop behavior |
| --- | --- |
| Account identity | Ticket login and saved replacement-server sessions are implemented. PSN sign-in remains unavailable. |
| Watermelon community API | Client routes and replacement-server endpoints cover levels, search, metadata, ratings, follows, queues, albums, leaderboards, activity, cloud saves, and reports. Retail parity is still being tested. |
| Friends, featured content, news, EULA, translations | Loaded from the replacement server where configured. Results depend on server data. |
| Trophies, PlayStation Store, web browser, Vita Near, platform SNS | No desktop backend. Calls are registered but unavailable or inert. |
| PSN account restrictions and chat permissions | Desktop INI fallbacks; these are not queried from PSN. |

Run the probes under `tests/networking/` against a local server for client
checks. They do not establish retail parity or validate a genuine PSN ticket.
