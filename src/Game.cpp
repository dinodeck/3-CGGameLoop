#include "Game.h"

#include "Asset.h"
#include "DancingSquidLua.h"
#include "IAssetOwner.h"
#include "LuaState.h"
#include "ManifestAssetStore.h"
#include "Settings.h"

Game::Game(Settings* settings, ManifestAssetStore* assetStore) :
    mReloadCount(0),
    mLuaState(NULL),
    mReady(false),
    mSettings(settings),
    mAssetStore(assetStore)
{
    mLuaState = new LuaState("Game");
    Game::Bind(mLuaState);
}

Game::~Game()
{
    if(mLuaState)
    {
        delete mLuaState;
        mLuaState = NULL;
    }
}

bool Game::OnAssetReload(Asset& asset)
{
    printf("Script asset should be reloaded: [%s]\n", asset.Name().c_str());
    mReloadCount++;
    return true;
}

void Game::OnAssetDestroyed(Asset& asset)
{
    // If you remove a script, that means we'll need to reload.
    mReloadCount++;
}


void Game::Reset()
{
    printf("Going to reload the lua state.\n");
    mLuaState->Reset();
    Game::Bind(mLuaState);

    // Main Script Id, needs to be taken from the asset store.
    // Settings->mainScript
    const char* mainScriptName = mSettings->mainScript.c_str();
    if(!mAssetStore->AssetExists(mainScriptName))
    {
        printf("Main script [%s], definied in settings.lua, does not exist in asset store.\n",
               mainScriptName);
        mReady = false;
        return;
    }

    Asset* mainScript = mAssetStore->GetAssetByName(mainScriptName);
    printf("Calling RunScript %s", mainScriptName);
    if(mLuaState->DoFile(mainScript->Path().c_str()))
    {
        printf("Reload success:\n");
        mReady = true;
    }
    else
    {
        printf("Reload failed:");
        printf("\tPress F2 to reload.\n");
        mReady = false;
    }
}

void Game::Update()
{
    if(!mReady)
    {
        return;
    }

    // // // Update the input
    // // for(std::vector<Pad*>::iterator it = mGamePads.begin(); it != mGamePads.end(); ++it)
    // // {
    // //     (*it)->Update();
    // // }
    // mouse.Update(deltaTime);
    // keyboard.Update(deltaTime);


    // // This should be in the render function
    // for(std::vector<Renderer*>::iterator it = Renderer::mRenderers.begin(); it !=Renderer::mRenderers.end(); ++it)
    // {
    //     (*it)->Render();
    // }

    bool result = mLuaState->DoString("update()");

    if(!result)
    {
        printf("Press F2 to reload.\n");
        mReady = false;
    }

    // Force a full collect each frame
    mLuaState->CollectGarbage();

}


int lua_load_library(lua_State* state)
{
    if(1 != lua_gettop(state) || !lua_isstring(state,  -1))
    {
        lua_pushstring(state, "Load Library: library name expected.\n");
        return lua_error(state);
    }

    // Get the library name passed in.
    const char* strLibName = lua_tostring(state,  -1);
    printf("Requested loading:[%s]\n", strLibName);
    lua_pop(state, 1); // clear the string.
    return 0;
}

void Game::Bind(LuaState* state)
{
    // Bind functions such as load library.
    lua_register(state->State(), "LoadLibrary", lua_load_library);
}