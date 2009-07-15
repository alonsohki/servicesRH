CREATE TABLE memo (
	owner		INT					NOT NULL,
	id			INT					NOT NULL,
	message		TEXT				NOT NULL,
	sent		TIMESTAMP			NOT NULL,
	source		VARCHAR ( 64 )		NOT NULL,
	isread		ENUM ( 'Y', 'N' )   NOT NULL DEFAULT 'N',
	
	PRIMARY KEY ( owner, id ),
	KEY ( owner ),
	FOREIGN KEY ( owner ) REFERENCES account ( id )
		ON DELETE CASCADE ON UPDATE CASCADE
) engine=InnoDB;