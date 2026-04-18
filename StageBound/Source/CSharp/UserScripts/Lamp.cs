using Owala.ScriptHost;

public class Lamp : OwalaBehaviour
{
	float moveSpeed = 0.1f;
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
