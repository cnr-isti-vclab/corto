## Using Corto within Unity

To use Corto within Unity, you first need to add the plugin to your project. Take the plugin (cortocodec_unity.dll in
your build directory) and add it to Assets/Plugins/ in your Unity project directory.

Then activate the use of 'unsafe' code within Unity:
- For Unity 2019 : 
	Go to Edit > Project Settings > Player > Other Settings
	Tick "Allow 'unsafe' Code" under "Configuration"

## Creating a script that use Corto

Add the following lines to the script that will call the plugin:

	[DllImport("cortocodec_unity", EntryPoint = "CreateDecoder")]
	private static extern IntPtr CreateDecoder(int length, byte[] data, [In, Out] Vector2[] decoderInfo);
	[DllImport("cortocodec_unity", EntryPoint = "DestroyDecoder")]
	private static extern void DestroyDecoder(IntPtr decoder);
	[DllImport("cortocodec_unity", EntryPoint = "DecodeMesh")]
	private static extern int DecodeMesh(IntPtr decoder, [In, Out] Vector3[] vertices, [In, Out] int[] indices, [In, Out] Vector3[] normals, [In, Out] Color[] colors, [In, Out] Vector2[] texcoord);

NOTE : Because we manipulate pointers, you have to declare the class as 'unsafe'

## Demo

You can find an example of script in corto/unity/
	- Create an empty game object
	- Add a MeshFilter and a MeshRenderer
	- Attach CortoDecodingObject.cs to it
	- Give it the .crt file name you want to open (ex: bunny)
	[NOTE : The file as to be a .bytes and must be placed in Assets/Resources/ !]
	- Press "Play"