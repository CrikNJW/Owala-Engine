using Owala.ScriptHost;

public class Lamp : OwalaBehaviour
{
	public float moveSpeed = 0.5f;
    public void OnCreate()
    {
        Log("Lamp created");
    }

    public void OnUpdate()
    {
        if (GetPosition(out float x, out float y))
        {
            SetPosition(x - moveSpeed, y);
        }
    }
}
