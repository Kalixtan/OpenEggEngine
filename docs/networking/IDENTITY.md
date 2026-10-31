# Online identity

OpenEgg can sign in with a replacement-server account without a PSN or RPCN
ticket. It can also send a configured PSN/RPCN ticket to the replacement
server; verified ticket identity stays separate from the editable display
name. OpenEgg rejects sessions the server marks unauthorized.

OpenEgg account login uses a separate server extension and accepts accounts
with no linked PlayStation identity. The `openegg_client` response field marks
that extension's response format; it is not authentication. Ticket login still
requires a valid signature for remote servers.

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

`OnlineCoordinator_GetOnlineID()` returns the active platform/account display
identity.
`Watermelon_GetUserIdentityId()` returns the replacement server's internal user
ID; scripts should use it for Watermelon request paths.

The native sign-in form creates or signs in to a replacement-server account.
With “Remember me” enabled, credentials are written to the Git-ignored
`login.ini`; this is a plain-text password file. Remote account login and ticket
login require HTTPS; plain HTTP is allowed only on loopback for development.
The manual `session_id` and `user_id` test bypass is accepted only for loopback
servers.

Remembered OpenEgg account details are stored in `login.ini`. Verified retail
ticket sessions are stored separately in `OpenEgg.ini`. Keep both files
private. On Unix, OpenEgg restricts credential-bearing files to owner read and
write; on Windows, protect them with file permissions. Local fixtures do not
verify a user-provided ticket or prove ownership of a real PSN identity.
