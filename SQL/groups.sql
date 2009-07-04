CREATE TABLE groups (
	name		VARCHAR ( 32 )		NOT NULL,
	id			INT					NOT NULL,
	
	PRIMARY KEY ( name ),
	FOREIGN KEY ( id ) REFERENCES account ( id ) ON DELETE CASCADE ON UPDATE CASCADE
) engine=InnoDB;