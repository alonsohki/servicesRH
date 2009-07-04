CREATE TABLE account (
	id			INT ( 9 )			NOT NULL auto_increment,
	name		VARCHAR ( 64 )		NOT NULL,
	password	VARCHAR ( 32 )		NOT NULL,
	email		VARCHAR ( 64 )		NULL,
	
	username	VARCHAR ( 64 )		NOT NULL,
	hostname	VARCHAR ( 128 )		NOT NULL,
	fullname	VARCHAR ( 128 )		NOT NULL,
	
	registered	TIMESTAMP			NOT NULL DEFAULT '0000-00-00 00:00:00',
	lastSeen	TIMESTAMP			NOT NULL DEFAULT '0000-00-00 00:00:00',
	
	PRIMARY KEY ( id ),
	KEY ( name, password ),
	UNIQUE ( name )
) engine=InnoDB;