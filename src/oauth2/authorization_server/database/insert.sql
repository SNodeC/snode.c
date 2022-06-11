set @password_salt1 = UUID();
insert into client(email, password_hash, password_salt) values(
    'daniel@flockert.at',
    SHA2(CONCAT(@password_salt1, 'rathalin'), 256),
    @password_salt1
);
