CREATE TABLE clones (
	name		VARCHAR ( 64 )		NOT NULL,
	amount		INT					NOT NULL,
	owner		INT					NULL,
	ip			VARCHAR ( 16 )		NULL,
	
	PRIMARY KEY ( name ),
	FOREIGN KEY ( owner ) REFERENCES account ( id )
		ON DELETE SET NULL ON UPDATE CASCADE,
	UNIQUE ( ip )
) engine=InnoDB;