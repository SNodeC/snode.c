insert into token values();
insert into token values();

set @password_salt1 = UUID();
insert into client(email, password_hash, password_salt, access_token_id, refresh_token_id) values(
    'daniel@flockert.at',
    SHA2(CONCAT(@password_salt1, 'rathalin'), 256),
    @password_salt1,
    (select id from token limit 1 offset 0),
    (select id from token limit 1 offset 1)
);
