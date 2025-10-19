class SPC_ArtilleryWeaponMuzzlePointsClass: ScriptComponentClass {}

class SPC_ArtilleryWeaponMuzzlePoints: ScriptComponent
{
	[Attribute()]
	ref array<ref PointInfo> m_aMuzzleEndPositions;
	
	override event protected void OnPostInit(IEntity owner)
	{
		foreach(PointInfo muzzleEnd : m_aMuzzleEndPositions)
		{
			muzzleEnd.Init(owner);
		}
	}
}