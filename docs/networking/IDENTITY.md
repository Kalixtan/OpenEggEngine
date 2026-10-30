# Online identity

OpenEgg sends the configured ticket to the replacement server. It does not
connect to PSN. The server uses the ticket's Online ID as the account identity
and requires a valid signature by default to prove ownership. That identity is
stored separately from the editable server display name. OpenEgg rejects
sessions the server marks unauthorized.

`Network_IsSignedIn()` and `Network_IsOnline()` keep the retail script-facing
state for an authenticated replacement-server session. They do not report a
PSN connection. Use these OpenEgg queries to tell the states apart:

- `OpenEgg_Network_IsReplacementServerAuthenticated()` is true after server
  login and identity validation.
- `OpenEgg_Network_IsPSNSignedIn()` is false; OpenEgg has no PSN provider.

Set `psn_online_id` and either `psn_ticket_hex` or `ticket_file` in
`OpenEgg.ini`. Treat the ticket as a secret. Ticket login requires HTTPS for
remote servers; plain HTTP is allowed only on loopback for development. A
server configured to accept unsigned tickets cannot verify account ownership;
use that setting only with a disposable local test server. The manual
`session_id` and `user_id` test bypass is accepted only for loopback servers.

After a verified login, OpenEgg stores the replacement-server session and
server user ID in the INI. It validates the session against the Online ID and
renews it for one year on use. This lets later runs connect without a PSN
ticket. Keep the INI private; the saved session is a bearer credential.
