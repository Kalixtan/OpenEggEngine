# Online identity

OpenEgg sends the configured ticket to the replacement server. It does not
connect to PSN. The server uses the ticket's Online ID as the account name;
ticket signature checks must be enabled on the server to prove ownership.

`Network_IsSignedIn()` and `Network_IsOnline()` keep the retail script-facing
state for an authenticated replacement-server session. They do not report a
PSN connection. Use these OpenEgg queries to tell the states apart:

- `OpenEgg_Network_IsReplacementServerAuthenticated()` is true after server
  login and identity validation.
- `OpenEgg_Network_IsPSNSignedIn()` is false; OpenEgg has no PSN provider.

Set `psn_online_id` and either `psn_ticket_hex` or `ticket_file` in
`OpenEgg.ini`. Treat the ticket as a secret. Ticket login requires HTTPS for
remote servers; plain HTTP is allowed only on loopback for development. A
server configured to accept unsigned tickets cannot verify account ownership.
