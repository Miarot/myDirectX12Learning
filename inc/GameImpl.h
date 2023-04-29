#pragma once 

#include <DX12LibPCH.h>

#include <Game.h>
#include <Window.h>

class GameImpl : public Game {
public:
	using super = Game;

    GameImpl(const std::wstring& name, int width, int height, bool vSync = false);

    /**
     *  Load content required for the demo.
     */
    virtual bool LoadContent() override;

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    virtual void UnloadContent() override;

protected:
    /**
     *  Update the game logic.
     */
    virtual void OnUpdate(UpdateEventArgs& e) override;

    /**
     *  Render stuff.
     */
    virtual void OnRender(RenderEventArgs& e) override;

    /**
     * Invoked by the registered window when a key is pressed
     * while the window has focus.
     */
    virtual void OnKeyPressed(KeyEventArgs& e) override;

private:
    // Transition a resource
    void TransitionResource(
        ComPtr<ID3D12GraphicsCommandList2> commandList,
        ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES beforeState, 
        D3D12_RESOURCE_STATES afterState
    );

    // Clear a render target view.
    void ClearRTV(
        ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv, 
        FLOAT* clearColor
    );

    uint64_t m_FenceValues[Window::BufferCount] = {};
};