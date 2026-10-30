# Online identity

OpenEgg sends the configured ticket to the replacement server. It does not
connect to PSN. The server uses the ticket's Online ID as the account identity
and requires a valid signature by default to prove ownership. That identity is
stored separately from the editable server display name. OpenEgg rejects
sessions the server marks unauthorized.

An existing registered server account must have PSN authentication enabled
before its Online ID can be linked. This prevents a ticket from claiming an
unrelated account that happens to use the same display name.
For remote servers, login responses must include the stable PSN identity field;
only loopback development servers may use the display-name fallback.

`Network_IsSignedIn()` and `Network_IsOnline()` keep the retail script-facing
state for an authenticated replacement-server session. They do not report a
PSN connection. `OnlineCoordinator_IsLoggedIntoPlatform()` remains false
because OpenEgg has no PSN sign-in provider. Use these OpenEgg queries to tell
the states apart:

The two `Network_*` values become true after replacement-server login and clear
when `OnlineCoordinator_GoOffline()` ends the active session. The remembered
server token remains in the INI for the next `GoOnline()` request.

- `OpenEgg_Network_IsReplacementServerAuthenticated()` is true after server
  login and identity validation.
- `OpenEgg_Network_IsPSNSignedIn()` is false; OpenEgg has no PSN provider.

`OnlineCoordinator_GetOnlineID()` returns the configured PSN Online ID.
`Watermelon_GetUserIdentityId()` returns the replacement server's internal user
ID; scripts should use it for Watermelon request paths.

Set `psn_online_id` and either `psn_ticket_hex` or `ticket_file` in
`OpenEgg.ini`. The Online ID must start with a letter and contain 3–16 letters,
digits, hyphens, or underscores. Treat the ticket as a secret. Remote ticket
login requires HTTPS; plain HTTP is allowed only on loopback for development. A
server configured with authentication disabled cannot verify account ownership;
use that setting only with a disposable local test server. The manual
`session_id` and `user_id` test bypass is accepted only for loopback servers.

After a verified login, OpenEgg stores the replacement-server session and
server user ID in the INI. It validates the session against the Online ID and
renews it for one year on use. The server allows this only for sessions minted
from verified PS3/Vita tickets. Local integration fixtures exercise the ticket
path but do not verify a user-provided ticket or prove ownership of a real PSN
identity. Later runs can connect without a PSN ticket after real verification.
Keep the INI private; the saved session is a bearer credential.
On Unix, OpenEgg restricts the INI to owner read/write when it loads a ticket
or session setting. It also restricts a ticket file to owner read/write before
loading it. On Windows, keep both files private with file permissions.
