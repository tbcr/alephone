/*
IMPORT_DEFINITIONS.C
Sunday, October 2, 1994 1:25:23 PM  (Jason')

Aug 12, 2000 (Loren Petrich):
	Using object-oriented file handler
*/

#include "cseries.h"
#include <string.h>

#include "tags.h"
#include "map.h"
#include "interface.h"
#include "game_wad.h"
#include "wad.h"
#include "game_errors.h"
// LP addition
#include "FileHandler.h"

/* ---------- globals */

/* sadly extern'ed from their respective files */
extern byte monster_definitions[];
extern byte projectile_definitions[];
extern byte effect_definitions[];
extern byte weapon_definitions[];
extern byte physics_models[];

#define IMPORT_STRUCTURE
#include "extensions.h"

/* ---------- local globals */
static FileSpecifier PhysicsFileSpec;

/* ---------- local prototype */
static struct wad_data *get_physics_wad_data(bool *bungie_physics);
static void import_physics_wad_data(struct wad_data *wad);

/* ---------- code */
void set_physics_file(FileSpecifier& File)
{
	PhysicsFileSpec = File;
	// memcpy(&physics_file, file, sizeof(FileDesc));
	
	return;
}

void set_to_default_physics_file(
	void)
{
	get_default_physics_spec(PhysicsFileSpec);

//	dprintf("Set to: %d %d %.*s", physics_file.vRefNum, physics_file.parID, physics_file.name[0], physics_file.name+1);

	return;
}

void import_definition_structures(
	void)
{
	struct wad_data *wad;
	bool bungie_physics;
	static bool warned_about_physics= false;

	wad= get_physics_wad_data(&bungie_physics);
	if(wad)
	{
		if(!bungie_physics && !warned_about_physics)
		{
			/* warn the user that external physics models are Bad Thing� */
			alert_user(infoError, strERRORS, warningExternalPhysicsModel, 0);
			warned_about_physics= true;
		}
		
		/* Actually load it in.. */		
		import_physics_wad_data(wad);
		
		free_wad(wad);
	}

	return;
}

void *get_network_physics_buffer(
	long *physics_length)
{
	void *data= get_flat_data(PhysicsFileSpec, false, 0);
	
	if(data)
	{
		*physics_length= get_flat_data_length(data);
	} else {
		*physics_length= 0;
	}
	
	return data;
}

void process_network_physics_model(
	void *data)
{
	if(data)
	{
		struct wad_header header;
		struct wad_data *wad;
	
		wad= inflate_flat_data(data, &header);
		if(wad)
		{
			import_physics_wad_data(wad);
			free_wad(wad); /* Note that the flat data points into the wad. */
		}
	}
	
	return;
}

/* --------- local code */
static struct wad_data *get_physics_wad_data(
	bool *bungie_physics)
{
	struct wad_data *wad= NULL;
	
//	dprintf("Open is: %d %d %.*s", physics_file.vRefNum, physics_file.parID, physics_file.name[0], physics_file.name+1);

	OpenedFile PhysicsFile;
	if(open_wad_file_for_reading(PhysicsFileSpec,PhysicsFile));
	{
		struct wad_header header;

		if(read_wad_header(PhysicsFile, &header))
		{
			if(header.data_version==BUNGIE_PHYSICS_DATA_VERSION || header.data_version==PHYSICS_DATA_VERSION)
			{
				wad= read_indexed_wad_from_file(PhysicsFile, &header, 0, true);
				if(header.data_version==BUNGIE_PHYSICS_DATA_VERSION)
				{
					*bungie_physics= true;
				} else {
					*bungie_physics= false;
				}
			}
		}

		close_wad_file(PhysicsFile);
	} 
	
	/* Reset any errors that might have occurred.. */
	set_game_error(systemError, errNone);

	return wad;
}

static void import_physics_wad_data(
	struct wad_data *wad)
{
	short index;
	
	for(index= 0; index<NUMBER_OF_DEFINITIONS; ++index)
	{
		long length;
		struct definition_data *definition= definitions+index;
		void *data;			

		/* Given a wad, extract the given tag from it */
		data= extract_type_from_wad(wad, definition->tag, &length);
		if(data)
		{
			/* Copy it into the proper array */
			memcpy(definition->data, data, length);
		}
	}
	
	return;
}

