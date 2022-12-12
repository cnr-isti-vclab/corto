using System.Collections.Generic;
using UnityEngine;

namespace Corto
{
    public class CortoDecodingObject : MonoBehaviour
    {
        #region - FIELDS
        [SerializeField] private string _crtFileName;
        #endregion

        #region - MONOBEHAVIOUR
        private void Start()
        {
            // TODO: Find how to handle multiple textures in a single geometry
            var meshes = new List<Mesh>();
            var loader = new CortoMeshLoader();

            int numFaces = loader.LoadMeshFromAsset(_crtFileName+".crt", ref meshes);

            if(numFaces > 0)
            {
                Debug.Log("Mesh fully decoded. Loading mesh to Unity.");
                GetComponent<MeshFilter>().mesh = meshes[0];
                // Materials are unsupported for the moment so we apply a default material
                GetComponent<Renderer>().material = new Material(Shader.Find("Standard"));
            }
        }
        #endregion
    }
}