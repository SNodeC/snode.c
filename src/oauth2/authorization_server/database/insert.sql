insert into token values();
insert into token values();
insert into client(access_token_id, refresh_token_id) values(
    (select id from token limit 1 offset 0),
    (select id from token limit 1 offset 1)
);
