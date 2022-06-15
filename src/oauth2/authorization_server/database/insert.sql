set @password_salt1 = UUID();
insert into client(email, password_hash, password_salt) values(
    'daniel@flockert.at',
    SHA1(CONCAT(@password_salt1, 'rathalin')),
    @password_salt1
);
