select
    c.id,
    c.uuid 'client_id',
    c.email 'email',
    c.password_hash 'password_hash',
    c.password_salt 'password_salt',
    at.uuid 'access_token',
    at.expire_date 'access_token_expire_date',
    rt.uuid 'refresh_token',
    rt.expire_date 'refresh_token_expire_date'
from client c
join token at
    on c.access_token_id = at.id
join token rt
    on c.refresh_token_id = rt.id;
