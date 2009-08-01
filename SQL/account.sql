CREATE TABLE account (
	id			INT ( 9 )			NOT NULL auto_increment,
	name		VARCHAR ( 64 )		NOT NULL,
	password	VARCHAR ( 32 )		NOT NULL,
	email		VARCHAR ( 64 )		NULL,
	lang		VARCHAR ( 8 )		NOT NULL DEFAULT 'EN',
	rank		INT					NOT NULL DEFAULT '-1',
	
	suspended	VARCHAR ( 256 )		NULL,
	suspendExp	TIMESTAMP			NULL,
	
	username	VARCHAR ( 64 )		NOT NULL,
	hostname	VARCHAR ( 128 )		NOT NULL,
	fullname	VARCHAR ( 128 )		NOT NULL,
	quitmsg		VARCHAR ( 256 )		NULL,
	
	vhost		VARCHAR ( 32 )		NULL,
	web			VARCHAR ( 128 )		NULL,
	greetmsg	VARCHAR ( 150 )		NULL,
	
	registered	TIMESTAMP			NOT NULL DEFAULT '0000-00-00 00:00:00',
	lastSeen	TIMESTAMP			NOT NULL DEFAULT '0000-00-00 00:00:00',
	
	private		ENUM('Y', 'N')		NOT NULL DEFAULT 'N',
	
	PRIMARY KEY ( id ),
	KEY ( name, password ),
	KEY ( name ),
	UNIQUE ( name )
) engine=InnoDB;