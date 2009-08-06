CREATE TABLE gline (
	id				INT					NOT NULL auto_increment,
	mask			VARCHAR ( 256 )		NOT NULL,
	compiledMask	VARCHAR ( 512 )		NOT NULL,
	compiledLen		INT					NOT NULL,
	`from`			VARCHAR ( 64 )		NOT NULL,
	expiration		TIMESTAMP			NOT NULL,
	reason			VARCHAR ( 256 )		NOT NULL,
	
	PRIMARY KEY ( id ),
	KEY ( mask ), UNIQUE ( mask )
) engine=MyISAM;