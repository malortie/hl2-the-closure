//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "../hud_crosshair.h"

#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Draws the zoom screen
//-----------------------------------------------------------------------------
class CHudScope : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE(CHudScope, vgui::Panel);

public:
	CHudScope(const char *pElementName);
	
	bool	ShouldDraw( void );
	void	Init( void );
	void	LevelInit( void );

	void	MsgFunc_Scope(bf_read &msg);

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint( void );

private:
	bool	m_bZoomOn;
	CMaterialReference m_ZoomMaterial;
};

DECLARE_HUDELEMENT(CHudScope);
DECLARE_HUD_MESSAGE(CHudScope, Scope);

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScope::CHudScope(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudZoom")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: standard hud element init function
//-----------------------------------------------------------------------------
void CHudScope::Init(void)
{
	HOOK_HUD_MESSAGE(CHudScope, Scope);

	m_bZoomOn = false;

	m_ZoomMaterial.Init("vgui/white_additive", TEXTURE_GROUP_VGUI);
}

//-----------------------------------------------------------------------------
// Purpose: standard hud element init function
//-----------------------------------------------------------------------------
void CHudScope::LevelInit(void)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: sets scheme colors
//-----------------------------------------------------------------------------
void CHudScope::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
	SetFgColor(scheme->GetColor("ZoomReticleColor", GetFgColor()));

	SetForceStereoRenderToFrameBuffer( true );
	int x, y;
	int screenWide, screenTall;
	surface()->GetFullscreenViewport( x, y, screenWide, screenTall );
	SetBounds(0, 0, screenWide, screenTall);
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudScope::ShouldDraw(void)
{
	return ( m_bZoomOn && CHudElement::ShouldDraw());
}

#define	ZOOM_FADE_TIME	0.4f
//-----------------------------------------------------------------------------
// Purpose: draws the zoom effect
//-----------------------------------------------------------------------------
void CHudScope::Paint(void)
{

	// see if we're zoomed any
	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>(C_BasePlayer::GetLocalPlayer());
	if ( pPlayer == NULL )
		return;


	// draw the darkened edges, with a rotated texture in the four corners
	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, m_ZoomMaterial);
	if (!pMesh)
		return;

	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);

	meshBuilder.Color4ub(255, 0, 0, 255);
	meshBuilder.TexCoord2f(0, 0, 0);
	meshBuilder.Position3f(0, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(0, 255, 0, 255);
	meshBuilder.TexCoord2f(0, 1, 0);
	meshBuilder.Position3f(0, 512, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(0, 0, 255, 255);
	meshBuilder.TexCoord2f(0, 1, 1);
	meshBuilder.Position3f(0, 512, 512);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(255, 0, 255, 255);
	meshBuilder.TexCoord2f(0, 0, 1);
	meshBuilder.Position3f(0, 0, 255);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();
}



//-----------------------------------------------------------------------------
// Purpose: Message handler for Damage message
//-----------------------------------------------------------------------------
void CHudScope::MsgFunc_Scope(bf_read &msg)
{
	int bScopeActive = msg.ReadByte();	// on/off

	m_bZoomOn = (bScopeActive == 1) ? true : false;
}