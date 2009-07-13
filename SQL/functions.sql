CREATE FUNCTION LOWERNAME ( name CHAR(64) )
RETURNS CHAR(64)
DETERMINISTIC
RETURN REPLACE ( REPLACE ( REPLACE ( REPLACE ( LOWER ( name ), '[', '{' ), ']', '}' ), '^', '~' ), '\\', '|');