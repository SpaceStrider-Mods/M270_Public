// ----------------------------------------------
// SPC_ArtilleryComputerComponentClass Definition
// ----------------------------------------------
[EntityEditorProps(category: "Components", description: "Artillery computer with turret stabilization")]
class SPC_ArtilleryComputerComponentClass : ScriptGameComponentClass {}

// ----------------------------------------------
// SPC_ArtilleryComputerComponent
// ----------------------------------------------
class SPC_ArtilleryComputerComponent : ScriptComponent
{
	// --- Component References ---
	protected SCR_MapEntity m_MapEntity;
	protected IEntity m_eOwner;
	protected TurretControllerComponent m_TurretController;
	protected VehicleWheeledSimulation m_VehicleSim;

	// --- Aiming and Stabilization State ---
	protected vector m_vAimOrigin;
	protected vector m_vOldAimRotation;
	protected vector m_vCompensatedTarget;
	protected bool m_bStabilizationEnabled = false;
	protected int m_iMuzzleIndex = 0;
	protected float m_fCurrentBallisticTime = 0.0;

	// --- Configurable Attributes ---
	[Attribute(defvalue: "-10.0", desc: "Time the fuze will go off before impact for cluster.")]
	protected float m_fFuzeTimeOffset;
	
	[Attribute(defvalue: "48.0", desc: "Horizontal stabilization speed in degrees per second.")]
	protected float m_fHorizontalStabilizationSpeed;
	
	[Attribute(defvalue: "16.0", desc: "Vertical stabilization speed in degrees per second.")]
	protected float m_fVerticalStabilizationSpeed;
	
	[Attribute(defvalue: "20.0", desc: "Vehicle speed [km/h] at which stabilization disables.")]
	protected float m_fDisableStabilizationSpeed;
	
	[Attribute("SPC_ToggleStabilization", desc: "Input action to toggle stabilization")]
	protected string m_sStabilizationToggleAction;

	#ifdef WORKBENCH
	ref Shape m_DebugLine;
	ref Shape m_DebugTarget;
	#endif

	// ----------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		m_eOwner = owner;
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_VehicleSim = VehicleWheeledSimulation.Cast(owner.FindComponent(VehicleWheeledSimulation));
	}

	// ----------------------------------------------
	int GetGlobalMuzzleIndex(array<IEntity> weaponEntities, int weaponIndex, int barrelIndex)
	{
		int offset = 0;
		for (int i = 0; i < weaponIndex; i++)
		{
			IEntity weapEnt = weaponEntities[i];
			SPC_ArtilleryWeaponMuzzlePoints points = SPC_ArtilleryWeaponMuzzlePoints.Cast(weapEnt.FindComponent(SPC_ArtilleryWeaponMuzzlePoints));
			if (points)
				offset += points.m_aMuzzleEndPositions.Count();
		}
		return offset + barrelIndex;
	}

	// ----------------------------------------------
	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		if (!m_bStabilizationEnabled || !m_TurretController)
			return;

		TurretComponent turretComp = m_TurretController.GetTurretComponent();
		if (!turretComp)
			return;

		IEntity turretEntity = turretComp.GetOwner();
		if (!turretEntity)
			return;

		vector turretMat[4];
		turretEntity.GetTransform(turretMat);

		float quat[4];
		Math3D.MatrixToQuat(turretMat, quat);
		float quatInv[4];
		Math3D.QuatInverse(quatInv, quat);

		BaseWeaponManagerComponent weaponManager = m_TurretController.GetWeaponManager();
		if (!weaponManager)
			return;

		array<IEntity> weaponEntities();
		weaponManager.GetWeaponsList(weaponEntities);
		BaseWeaponComponent weapon = weaponManager.GetCurrentWeapon();
		if (!weapon)
			return;

		IEntity currentWeaponEntity = weapon.GetOwner();
		int weaponIndex = -1;
		for (int i = 0; i < weaponEntities.Count(); i++)
		{
			if (weaponEntities[i] == currentWeaponEntity)
			{
				weaponIndex = i;
				break;
			}
		}
		if (weaponIndex == -1)
		{
			#ifdef WORKBENCH
			Print("Failed to resolve weapon index using GetWeaponsList.");
			#endif
			return;
		}

		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (!muzzle)
			return;

		int barrelIndex = muzzle.GetCurrentBarrelIndex();
		int muzzleIndex = GetGlobalMuzzleIndex(weaponEntities, weaponIndex, barrelIndex);

		IEntity weaponEntity = weapon.GetOwner();
		if (!weaponEntity)
			return;

		SPC_ArtilleryWeaponMuzzlePoints muzzlePoints = SPC_ArtilleryWeaponMuzzlePoints.Cast(weaponEntity.FindComponent(SPC_ArtilleryWeaponMuzzlePoints));
		if (!muzzlePoints || muzzleIndex >= muzzlePoints.m_aMuzzleEndPositions.Count())
			return;

		PointInfo currentMuzzle = muzzlePoints.m_aMuzzleEndPositions[muzzleIndex];
		vector fireOrigin = currentMuzzle.GetWorldTransformAxis(3);

		vector toTarget = m_vCompensatedTarget - fireOrigin;
		if (toTarget.LengthSq() < 0.01)
			return;

		vector dirLocal = SCR_Math3D.QuatMultiply(quatInv, toTarget);
		vector desiredAngles = dirLocal.Normalized().VectorToAngles();

		vector currentAngles = turretComp.GetAimingRotation();
		vector angleChange = desiredAngles - currentAngles;
		angleChange[0] = Math.MapAngle(angleChange[0]);
		angleChange[1] = Math.MapAngle(angleChange[1]);

		float maxYaw = m_fHorizontalStabilizationSpeed * timeSlice;
		float maxPitch = m_fVerticalStabilizationSpeed * timeSlice;
		angleChange[0] = Math.Clamp(angleChange[0], -maxYaw, maxYaw);
		angleChange[1] = Math.Clamp(angleChange[1], -maxPitch, maxPitch);

		vector newAngles = currentAngles;
		newAngles[0] = newAngles[0] + angleChange[0];
		newAngles[1] = newAngles[1] + angleChange[1];
		turretComp.SetAimingRotation(newAngles * Math.DEG2RAD);
		m_vOldAimRotation = newAngles;

		#ifdef WORKBENCH
		DebugDraw(fireOrigin);
		//PrintFormat("Debugging muzzle index: weapon %1, barrel %2, global %3", weaponIndex, barrelIndex, muzzleIndex);
		#endif
	}
	
	protected void RecalculateTrajectory()
	{
		if (!m_TurretController)
			return;

		BaseWeaponManagerComponent weaponManager = m_TurretController.GetWeaponManager();
		if (!weaponManager)
			return;

		array<IEntity> weaponEntities();
		weaponManager.GetWeaponsList(weaponEntities);
		BaseWeaponComponent weapon = weaponManager.GetCurrentWeapon();
		if (!weapon)
			return;

		IEntity currentWeaponEntity = weapon.GetOwner();
		int weaponIndex = -1;
		for (int i = 0; i < weaponEntities.Count(); i++)
		{
			if (weaponEntities[i] == currentWeaponEntity)
			{
				weaponIndex = i;
				break;
			}
		}
		if (weaponIndex == -1)
		{
			#ifdef WORKBENCH
			Print("Failed to resolve weapon index using GetWeaponsList.");
			#endif
			return;
		}

		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (!muzzle)
			return;

		int barrelIndex = muzzle.GetCurrentBarrelIndex();
		int muzzleIndex = GetGlobalMuzzleIndex(weaponEntities, weaponIndex, barrelIndex);

		IEntity weaponEntity = weapon.GetOwner();
		if (!weaponEntity)
			return;

		SPC_ArtilleryWeaponMuzzlePoints muzzlePoints = SPC_ArtilleryWeaponMuzzlePoints.Cast(weaponEntity.FindComponent(SPC_ArtilleryWeaponMuzzlePoints));
		if (!muzzlePoints || muzzleIndex >= muzzlePoints.m_aMuzzleEndPositions.Count())
			return;

		PointInfo currentMuzzle = muzzlePoints.m_aMuzzleEndPositions[muzzleIndex];
		vector fireOrigin = currentMuzzle.GetWorldTransformAxis(3);

		vector aimVector = m_vAimOrigin - fireOrigin;
		float distance = aimVector.Length();

		float heightCorrection = BallisticTable.GetAimHeightOfNextProjectile(distance, m_fCurrentBallisticTime, muzzle);
		
		ApplyFuzeToAllProjectiles();

		m_vCompensatedTarget = m_vAimOrigin;
		m_vCompensatedTarget[1] = m_vAimOrigin[1] + heightCorrection;
	}
	
	protected void ApplyFuzeToAllProjectiles()
	{
		if (!m_TurretController)
			return;
	
		BaseWeaponManagerComponent weaponManager = m_TurretController.GetWeaponManager();
		if (!weaponManager)
			return;
	
		array<IEntity> weaponEntities();
		weaponManager.GetWeaponsList(weaponEntities);
	
		foreach (IEntity weaponEntity : weaponEntities)
		{
			BaseWeaponComponent weapon = BaseWeaponComponent.Cast(weaponEntity.FindComponent(BaseWeaponComponent));
			if (!weapon)
				continue;
	
			array<BaseMuzzleComponent> muzzles();
			weapon.GetMuzzlesList(muzzles);
			
			foreach (BaseMuzzleComponent muzzle : muzzles)
			{
				if (!muzzle)
					continue;
			
				RocketEjectorMuzzleComponent rocketMuzzle = RocketEjectorMuzzleComponent.Cast(muzzle);
				if (!rocketMuzzle)
					continue;
			
				array<IEntity> loadedProjectiles();
				int barrelCount = rocketMuzzle.GetLoadedEntities(loadedProjectiles);
			
				for (int i = 0; i < barrelCount; i++)
				{
					IEntity proj = loadedProjectiles[i];
					if (!proj)
						continue;
			
					TimerTriggerComponent timerComp = TimerTriggerComponent.Cast(proj.FindComponent(TimerTriggerComponent));
					if (!timerComp)
						continue;
			
					float fuzeTime = m_fCurrentBallisticTime + m_fFuzeTimeOffset;
					timerComp.SetTimer(fuzeTime);
				}
			}
		}
	}
	
	// ----------------------------------------------
	void OpenComputer(SCR_MapEntity mapEntity, IEntity user)
	{
		SCR_ChimeraCharacter userCharacter = SCR_ChimeraCharacter.Cast(user);
		m_TurretController = TurretControllerComponent.Cast(userCharacter.GetCompartmentAccessComponent().GetCompartment().GetController());
		if (!m_TurretController) return;

		m_MapEntity = mapEntity;
		m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
		m_MapEntity.GetOnSelection().Insert(OnMapSelection);
		m_MapEntity.GetOnMapClose().Insert(OnMapClose);

		RegisterStabilizationToggle();
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.ArtilleryComputerMenu);
		SetEventMask(m_eOwner, EntityEvent.POSTFRAME);
	}

	protected void OnMapOpen(MapConfiguration config)
	{
		UnregisterStabilizationToggle();
	}

	protected void OnMapClose(MapConfiguration config)
	{
		m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
		m_MapEntity.GetOnSelection().Remove(OnMapSelection);
		RegisterStabilizationToggle();
	}

	void OnMapSelection(vector selectedPos)
	{
		float worldX, worldY, heightAtPos;
		m_MapEntity.ScreenToWorld(selectedPos[0], selectedPos[2], worldX, worldY);
		heightAtPos = m_MapEntity.GetWorld().GetSurfaceY(worldX, worldY);
		m_vAimOrigin = Vector(worldX, heightAtPos, worldY);
		RecalculateTrajectory();
		m_bStabilizationEnabled = true;
		SetEventMask(m_eOwner, EntityEvent.POSTFRAME);
	}
	
	#ifdef WORKBENCH
	void DebugDraw(vector fireOrigin, int barrelIndex = -1)
	{
		if (m_DebugLine)
			m_DebugLine = null;
		if (m_DebugTarget)
			m_DebugTarget = null;
	
		vector points[2] = { fireOrigin, m_vCompensatedTarget };
		m_DebugLine = Shape.CreateLines(Color.RED, ShapeFlags.NOZBUFFER | ShapeFlags.ONCE, points, 2);
		m_DebugTarget = Shape.CreateSphere(Color.GREEN, ShapeFlags.NOZBUFFER | ShapeFlags.ONCE, m_vCompensatedTarget, 0.3);
	
		if (barrelIndex >= 0)
			PrintFormat("Debugging barrel index: %1", barrelIndex);
	}
	#endif
	
	// ----------------------------------------------
	protected void OnToggleStabilization()
	{
		m_bStabilizationEnabled = false;
		ClearEventMask(m_eOwner, EntityEvent.POSTFRAME);
	}

	protected void RegisterStabilizationToggle()
	{
		GetGame().GetInputManager().AddActionListener(m_sStabilizationToggleAction, EActionTrigger.DOWN, OnToggleStabilization);
	}

	protected void UnregisterStabilizationToggle()
	{
		GetGame().GetInputManager().RemoveActionListener(m_sStabilizationToggleAction, EActionTrigger.DOWN, OnToggleStabilization);
	}

	override void OnDelete(IEntity owner)
	{
		UnregisterStabilizationToggle();
		super.OnDelete(owner);
	}
};
