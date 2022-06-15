delete from token where id not in (select access_token_id from client union select auth_code_id from client union select refresh_token_id from client);
