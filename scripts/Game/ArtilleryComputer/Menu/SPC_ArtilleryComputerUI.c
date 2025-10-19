class SPC_ArtilleryComputerUI: ChimeraMenuBase
{	
	
	protected SCR_MapEntity m_MapEntity;
	protected SCR_ChatPanel m_ChatPanel;
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{	
		
		if (!m_MapEntity)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{3C6C98B0E342CAA0}Configs/Map/MapArtilleryComputer.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);
			
		m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);

		InputManager inputMan = GetGame().GetInputManager();
		if (!inputMan)
			return;
		
		inputMan.AddActionListener("TutorialFastTravelMapMenuClose", EActionTrigger.DOWN, Close);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{		
		SCR_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
		if (m_MapEntity)
		{
			m_MapEntity.CloseMap();
			
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_HUD_MAP_CLOSE);
		}
		
		InputManager inputMan = GetGame().GetInputManager();
		if (!inputMan)
			return;
		
		inputMan.RemoveActionListener("TutorialFastTravelMapMenuClose", EActionTrigger.DOWN, Close);
	}
		
	//------------------------------------------------------------------------------------------------
	override void OnMenuInit()
	{		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
	}
	
	//------------------------------------------------------------------------------------------------
	void Callback_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		if (!m_ChatPanel.IsOpen())
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
	}
	//------------------------------------------------------------------------------------------------
	void OnMapOpen(MapConfiguration config)
	{
		m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_HUD_MAP_OPEN);
		
	}
};