CREATE TABLE channel (
	id				INT					NOT NULL auto_increment,
	name			VARCHAR ( 128 )		NOT NULL,
	password		VARCHAR ( 32 )		NOT NULL,
	description		VARCHAR ( 256 )		NOT NULL,
	
	registered		TIMESTAMP			NOT NULL DEFAULT '0000-00-00 00:00:00',
	lastUsed		TIMESTAMP			NOT NULL DEFAULT '0000-00-00 00:00:00',
	founder			INT					NULL,
	successor		INT					NULL,
	
	-- Opciones
	debug			ENUM ( 'Y', 'N' )	NOT NULL DEFAULT 'Y',
	
	PRIMARY KEY ( id ),
	KEY ( name ),
	UNIQUE ( name ),
	FOREIGN KEY ( founder ) REFERENCES account ( id )
		ON DELETE SET NULL ON UPDATE CASCADE,
	FOREIGN KEY ( successor ) REFERENCES account ( id )
		ON DELETE SET NULL ON UPDATE CASCADE
) engine=InnoDB;