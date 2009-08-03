CREATE TABLE `access` (
	account			INT			NOT NULL,
	channel			INT			NOT NULL,
	`level`			INT			NOT NULL,
	
	PRIMARY KEY ( account, channel ),
	KEY ( channel ),
	FOREIGN KEY ( account ) REFERENCES account ( id )
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY ( channel ) REFERENCES channel ( id )
		ON DELETE CASCADE ON UPDATE CASCADE
) engine=InnoDB;