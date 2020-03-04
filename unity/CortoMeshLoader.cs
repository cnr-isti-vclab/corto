using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

namespace Corto
{
    public unsafe class CortoMeshLoader
    {
        #region - METHODS
        public int LoadMeshFromAsset(string assetName, ref List<Mesh> meshes)
        {
            TextAsset asset = Resources.Load(assetName, typeof(TextAsset)) as TextAsset;

            if (asset == null)
            {
                Debug.LogWarning("Didn't load file !");
                return -1;
            }

            byte[] encodedData = asset.bytes;
            Debug.LogFormat("File loaded (size: {0}).", encodedData.Length.ToString());

            if (encodedData.Length == 0)
            {
                Debug.LogWarning("No encoded data !");
                return -1;
            }

            return DecodeMesh(encodedData, ref meshes);
        }

        private int DecodeMesh(byte[] data, ref List<Mesh> meshes)
        {
            Vector2[] decoderInfo = new Vector2[1]; // x = nface, y = nvert
            IntPtr decoder = CreateDecoder(data.Length, data, decoderInfo);

            int[] indices = new int[(int)decoderInfo[0].x * 3];
            Vector3[] vertices = new Vector3[(int)decoderInfo[0].y];
            Vector3[] normals = new Vector3[(int)decoderInfo[0].y];
            Color[] colors = new Color[(int)decoderInfo[0].y];
            Vector2[] uvs = new Vector2[(int)decoderInfo[0].y];

            System.Diagnostics.Stopwatch stopwatch = new System.Diagnostics.Stopwatch();
            stopwatch.Start();

            if(DecodeMesh(decoder, vertices, indices, normals, colors, uvs) <= 0)
            {
                Debug.LogWarning("Failed: Decoding error.");
                return -1;
            }
            stopwatch.Stop();

            TimeSpan ts = stopwatch.Elapsed;

            string elapsedTime = String.Format("{0:00}:{1:00}:{2:00}.{3:00}",
            ts.Hours, ts.Minutes, ts.Seconds,
            ts.Milliseconds / 10);

            Debug.LogFormat("Mesh decoding is complete in {0}", elapsedTime);
            Debug.LogFormat("Number of triangles: {0}", (indices.Length / 3).ToString());
            Debug.LogFormat("Number of vertices: {0}", vertices.Length.ToString());

            DestroyDecoder(decoder);
            decoderInfo[0] = Vector2.zero;

            // TODO : Use 16bit if it's enough for the mesh (low priority)
            Mesh mesh = new Mesh
            {
                indexFormat = UnityEngine.Rendering.IndexFormat.UInt32,
                vertices = vertices,
                triangles = indices
            };

            if (uvs.Length > 0)
            {
                mesh.uv = uvs;
            }

            if(normals.Length > 0)
            {
                //mesh.normals = normals;
                mesh.RecalculateNormals();
            }
            else
            {
                Debug.Log("Mesh does not have normals. Recomputing...");
                mesh.RecalculateNormals();
            }

            if(colors.Length > 0)
            {
                mesh.colors = colors;
            }

            meshes.Add(mesh);

            return mesh.triangles.Length / 3;
        }

        // TODO: If porting to WebGL, use the Javascript code
        [DllImport("cortocodec_unity", EntryPoint = "CreateDecoder")]
        private static extern IntPtr CreateDecoder(int length, byte[] data, [In, Out] Vector2[] decoderInfo);
        [DllImport("cortocodec_unity", EntryPoint = "DestroyDecoder")]
        private static extern void DestroyDecoder(IntPtr decoder);
        [DllImport("cortocodec_unity", EntryPoint = "DecodeMesh")]
        private static extern int DecodeMesh(IntPtr decoder, [In, Out] Vector3[] vertices, [In, Out] int[] indices, [In, Out] Vector3[] normals, [In, Out] Color[] colors, [In, Out] Vector2[] texcoord);
        #endregion
    }
}