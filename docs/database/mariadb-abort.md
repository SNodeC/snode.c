# MariaDB session abort

`MariaDBClient::abort()` cancels the current session without draining queued SQL or delivering further application callbacks. A callback already executing may finish; command and row storage remains alive until that callback returns. The client may be destroyed immediately after abort. Repeated abort on the detached client is harmless.

The connection replaces its existing closing flag with Active, Drain and Abort modes. Abort detaches both client/connection pointers, uses Connector/C's public `mariadb_cancel()` and spans the existing command-start event. At that event, owned commands/results and the sequence queue are cleared. Registered receivers are disabled through the existing SNode.C path, and normal destruction calls `mysql_close()`. Before descriptor registration, that command event directly completes destruction. No direct socket access or core API changes are introduced.

Execution and post-callback advancement are guarded against Abort. Destruction suppresses cancelled-session callbacks. Ordinary client destruction retains draining semantics. Explicit subsequent use of a retained client creates a new session; old cleanup cannot detach that replacement.

Abort does not acknowledge remote rollback or establish the outcome of an already-sent mutation or COMMIT. No replay, reconnect of cancelled commands or automatic transaction rollback is added. If native cancellation reports failure, a fixed warning is logged and controlled close is still scheduled; no remote outcome is inferred.

Focused tests use disposable socket-only MariaDB instances and an independent SQL reader. They cover no-session/repeated abort, abort before and during connect, connected/error callbacks, unread streaming rows, end-of-results, idle and waiting queries, queued transaction cancellation, explicit reuse, and unchanged normal draining. Native cancellation failure before a session exists is exercised; cancellation failure on an established session and precisely lost COMMIT/ROLLBACK acknowledgements are not deterministically injected. No installed-library or live-controller acceptance is claimed.
