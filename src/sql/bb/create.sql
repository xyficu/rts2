CREATE TABLE observatories (
	observatory_id	integer PRIMARY KEY NOT NULL,
	longitude	float8 NOT NULL, -- geographical informations
	latitude	float8 NOT NULL,
	altitude	float8 NOT NULL,
	apiurl		VARCHAR(100),
	apiuser		VARCHAR(50),
	apipassword	VARCHAR(50)
);

CREATE TABLE targets_observatories (
	observatory_id	integer REFERENCES observatories(observatory_id),
	tar_id		integer REFERENCES targets (tar_id),
	obs_tar_id	integer NOT NULL
);

CREATE TABLE bb_schedules (
	schedule_id	integer PRIMARY KEY,
	tar_id		integer REFERENCES targets (tar_id)
);

CREATE TABLE observatory_schedules (
	schedule_id	integer REFERENCES bb_schedules (schedule_id),
	observatory_id	integer REFERENCES observatories (observatory_id),
	state		integer NOT NULL,
	created		timestamp with time zone NOT NULL,
	last_update	timestamp with time zone,
	-- first time the observation can be scheduled
	sched_from      timestamp with time zone,
	sched_to        timestamp with time zone
);

CREATE TABLE observatory_observations (
	observatory_id	integer REFERENCES observatories (observatory_id),
	schedule_id	integer REFERENCES bb_schedules (schedule_id),
	obs_id 		integer NOT NULL,
	tar_id		integer REFERENCES targets (tar_id),
	obs_ra		float8,
	obs_dec		float8,
	obs_slew	timestamp with time zone,
	obs_start	timestamp with time zone,
	obs_end		timestamp with time zone,
	onsky		float8, -- total second of on-sky images
	good_images	integer,
	bad_images	integer
);

CREATE SEQUENCE bb_schedule_id;
