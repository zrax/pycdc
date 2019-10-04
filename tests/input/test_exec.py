testcode = 'a = 12'

exec testcode
exec testcode in globals()
exec testcode in globals(), locals()
