# Notes for project meeting 28.02.2022

## Questions

## Notes

MariaDBConnector:
- WER
- RER
- EECR
Listen to TCP connect (get socket opt)

MariaDBClient:
- WER
- RER
- EECR

TLSHandshake

unobserved -> mysql_close(&mysql);


## Meeting 07.03.2022

Add arguments to onConnect lambda function

MySqlClient darf out of scope gehen

MySqlExecutor (mit new angelegt)

MySqlClient connect -> erzeugt I_real_connect -> MySqlExecutor führt aus