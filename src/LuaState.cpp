#include "LuaState.h"
#include <assert.h>
#include <string.h>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}


LuaState::LuaState(const char* name) :
    mName(name),
    mLuaState(NULL)
{
    // lua_newstate( MemHandler, NULL ); <- can use this to get some mem stats
    Reset();
}


LuaState::~LuaState()
{
    lua_close(mLuaState);
}

LuaState* LuaState::GetWrapper(lua_State* luaState)
{
    return static_cast<LuaState*>(LuaState::GetFromRegistry(luaState, "this"));
}

int LuaState::LuaError(lua_State* luaState)
{
	LuaState* wrapper = LuaState::GetWrapper(luaState);
	assert(wrapper);
	wrapper->OnError();
	return 0;
}

void LuaState::OnError()
{
 	// Prints out the error and a stack trace
    printf("\n[LUASTATE|%s] Error: %s\n", mName, lua_tostring(mLuaState, -1));

    // Push the debug library on the stack
    lua_getfield(mLuaState, LUA_GLOBALSINDEX, "debug");
    if(!lua_istable(mLuaState, -1))
    {
        printf("[LUASTATE|%s] Debug library not loaded. Couldn't get stack trace\n", mName);
        lua_pop(mLuaState, 1); // pop the non-table
        return;
    }

    // Debug library is on the stack

    // A traceback is the most basic thing we can do but we can also
    // Get variable information and all that good stuff - it's just
    // a question of how to present it.

    // Push the traceback on to the stack as a field
    lua_getfield(mLuaState, -1, "traceback");

    if(!lua_isfunction(mLuaState, -1))
    {
        // If the traceback field doesn't exist
        printf("[LUASTATE|%s] Tried to get trackback but function doesnt exist.\n", mName);
        printf("[LUASTATE|%s] Have you overidden the default debug table?\n", mName);
        lua_pop(mLuaState, 2); // pop table and field
        return;
    }

    // Call function on the top of the stack, with no arguements, expecting one return.
    lua_call(mLuaState, 0, 1);
    printf("[LUASTATE|%s] %s\n", mName, lua_tostring(mLuaState, -1));

    return;
}

lua_State* LuaState::State() const
{
	return mLuaState;
}

unsigned int LuaState::ItemsInStack()
{
	return lua_gettop(mLuaState);
}

bool LuaState::DoString(const char* str)
{
	// Does this leave the stack messy in case or error (probaby doesn't matter!)
	lua_pushcfunction(mLuaState, LuaState::LuaError);
	int fail = luaL_loadstring(mLuaState, str);

	if(fail)
	{
		printf("\n[LUASTATE|%s] Error: %s\n", mName, lua_tostring(mLuaState, -1));
		return false;
	}
	else
	{
		// Execute the string on the stack
		// If anything goes wrong call the function under that
		bool result = lua_pcall(mLuaState, 0, LUA_MULTRET, -2) == 0;
		lua_pop(mLuaState, 1); // remove error function
		return result;
	}
}

bool LuaState::DoFile(const char* path)
{
	// No package management at the moment so this is simple
	lua_pushcfunction(mLuaState, LuaState::LuaError);
	int fail = luaL_loadfile(mLuaState, path);
	if(fail)
	{
		printf("\n[LUASTATE|%s] Error: %s\n", mName, lua_tostring(mLuaState, -1));
		return false;
	}
	else
	{
        printf("\nBefore dofile protected call\n");
		// Execute the string on the stack
		// If anything goes wrong call the function under that
		bool result = lua_pcall(mLuaState, 0, LUA_MULTRET, -2) == 0;
        printf("\nAfter dofile protected call\n");
		lua_pop(mLuaState, 1); // remove error function
		return result;
	}
}

std::string LuaState::GetString(const char* key, const char* defaultStr)
{
	lua_getfield(mLuaState, LUA_GLOBALSINDEX, key);
	const char* out = luaL_optlstring(mLuaState, -1, defaultStr, NULL);
	lua_pop(mLuaState, 1); // Remove key,
	return out;
}

int LuaState::GetInt(const char* key, int defaultInt)
{
	lua_getfield(mLuaState, LUA_GLOBALSINDEX, key);
	int out = luaL_optint (mLuaState, -1, defaultInt);
	lua_pop(mLuaState, 1); // Remove key,
	return out;
}

void LuaState::Reset()
{
    if(mLuaState)
    {
        lua_close(mLuaState);
        mLuaState = NULL;
    }
    mLuaState = lua_open();
    luaL_openlibs(mLuaState);
    // Push object instance pointer in the lua state
    // So for static functions (used by Lua) we can retrieve this object associated with a lua state
    InjectIntoRegistry("this", this);
}

void LuaState::CollectGarbage()
{
    lua_gc(mLuaState, LUA_GCCOLLECT, 0);
}

void LuaState::InjectIntoRegistry(const char* key, void* object)
{
    lua_pushlstring(mLuaState, key, strlen(key));
    lua_pushlightuserdata(mLuaState, object);       /* push value */
    lua_settable(mLuaState, LUA_REGISTRYINDEX);     /* registry[key] = value */
}

void* LuaState::GetFromRegistry(lua_State* state, const char* key)
{
    lua_pushlstring(state, key, strlen(key));
    // Needs sanity checks.
    lua_gettable(state, LUA_REGISTRYINDEX);  /* retrieve value */
    void* object = lua_touserdata(state, -1);
    lua_pop(state, 1); // pop the user data off the top of the stack
    return object;
}

