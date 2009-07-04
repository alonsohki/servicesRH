CREATE TABLE account (
	id			INT ( 9 )			NOT NULL auto_increment,
	name		VARCHAR ( 64 )		NOT NULL,
	password	VARCHAR ( 32 )		NOT NULL,
	
	username	VARCHAR ( 64 )		NULL,
	hostname	VARCHAR ( 128 )		NULL,
	fullname	VARCHAR ( 128 )		NULL,
	
	registered	TIMESTAMP			NULL,
	lastSeen	TIMESTAMP			NULL,
	
	PRIMARY KEY ( id ),
	KEY ( name, password ),
	UNIQUE ( name )
) engine=InnoDB;