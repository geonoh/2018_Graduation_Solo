using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Windows;
using System.IO;

public class MeshExtract : MonoBehaviour {
    private StreamWriter writer = null;
    private int nLevel = 0;

    void WriteTabs(int nLevel)
    {
        for (int i = 0; i < nLevel; i++) writer.Write("\t");
    }

    void WriteFrameInfo(int nLevel, Transform current)
    {
        WriteTabs(nLevel);
        writer.WriteLine("FrameName: " + current.gameObject.name);

        WriteTabs(nLevel+1);
        writer.Write("Transform: ");
        writer.Write(current.localPosition.x + " " + current.localPosition.y + " " + current.localPosition.z + " ");
        writer.Write(current.localEulerAngles.x + " " + current.localEulerAngles.y + " " + current.localEulerAngles.z + " ");
        writer.Write(current.localRotation.x + " " + current.localRotation.y + " " + current.localRotation.z + " " + current.localRotation.w + " ");
        writer.WriteLine(current.localScale.x + " " + current.localScale.y + " " + current.localScale.z);

        WriteTabs(nLevel+1);
        writer.Write("TransformMatrix: ");

        //Matrix4x4 localMatrix = current.localToWorldMatrix.inverse;
        //writer.Write(localMatrix.m00 + " " + localMatrix.m10 + " " + localMatrix.m20 + " " + localMatrix.m30 + " ");
        //writer.Write(localMatrix.m01 + " " + localMatrix.m11 + " " + localMatrix.m21 + " " + localMatrix.m31 + " ");
        //writer.Write(localMatrix.m02 + " " + localMatrix.m12 + " " + localMatrix.m22 + " " + localMatrix.m32 + " ");
        //writer.WriteLine(localMatrix.m03 + " " + localMatrix.m13 + " " + localMatrix.m23 + " " + localMatrix.m33);

        writer.Write(current.localToWorldMatrix.m00 + " " + current.localToWorldMatrix.m10 + " " + current.localToWorldMatrix.m20 + " " + current.localToWorldMatrix.m30 + " ");
        writer.Write(current.localToWorldMatrix.m01 + " " + current.localToWorldMatrix.m11 + " " + current.localToWorldMatrix.m21 + " " + current.localToWorldMatrix.m31 + " ");
        writer.Write(current.localToWorldMatrix.m02 + " " + current.localToWorldMatrix.m12 + " " + current.localToWorldMatrix.m22 + " " + current.localToWorldMatrix.m32 + " ");
        writer.WriteLine(current.localToWorldMatrix.m03 + " " + current.localToWorldMatrix.m13 + " " + current.localToWorldMatrix.m23 + " " + current.localToWorldMatrix.m33);

        MeshFilter meshFilter = current.gameObject.GetComponent<MeshFilter>();
        if (meshFilter)
        {
            WriteTabs(nLevel+1);
            writer.WriteLine("MeshName: " + meshFilter.name);

            Mesh mesh = meshFilter.sharedMesh;

            Vector3[] pVertices = mesh.vertices; //Mesh.GetVertices(List<Vector3> vertices);
            Vector3[] pNormals = mesh.normals; //Mesh.GetNormals(List<Vector3> normals);
            Vector4[] pTangents = mesh.tangents; //int[] Mesh.GetTriangles(int submesh); 
            Vector2[] pUV1s = mesh.uv; //Mesh.GetUVs(0, List<Vector2> uvs);
            Vector2[] pUV2s = mesh.uv2;
            Vector2[] pUV3s = mesh.uv3;
            Vector2[] pUV4s = mesh.uv4;
            int[] pIndices = mesh.triangles; //int[] Mesh.GetTriangles(int submesh);

            Color[] pColors = mesh.colors; //Mesh.GetColors(List<Color> colors); 
            Bounds bounds = mesh.bounds; //Bounding Box

            int nVertices = mesh.vertexCount;
            int nVertexBuffers = mesh.vertexBufferCount;
            //IntPtr Mesh.GetNativeVertexBufferPtr(int index); 

            int nSubMeshes = mesh.subMeshCount;
            //uint Mesh.GetIndexStart(int submesh); 
            //uint Mesh.GetIndexCount(int submesh);
            //int[] Mesh.GetIndices(int submesh); 

            if (pVertices != null)
            {
                WriteTabs(nLevel + 1);
                writer.WriteLine("Vertices: " + pVertices.Length);
                WriteTabs(nLevel + 1);
                foreach (Vector3 v in pVertices) writer.Write(v.x + " " + v.y + " " + v.z + " ");
                writer.WriteLine(" ");
            }

            if (pNormals != null)
            {
                WriteTabs(nLevel + 1);
                writer.WriteLine("Normals: " + pNormals.Length);
                WriteTabs(nLevel + 1);
                foreach (Vector3 n in pNormals) writer.Write(n.x + " " + n.y + " " + n.z + " ");
                writer.WriteLine(" ");
            }

            if (pIndices != null)
            {
                WriteTabs(nLevel + 1);
                writer.WriteLine("Indices: " + pIndices.Length);
                WriteTabs(nLevel + 1);
                foreach (int i in pIndices) writer.Write(i + " ");
                writer.WriteLine(" ");
            }

            if (pUV1s != null)
            {
                WriteTabs(nLevel + 1);
                writer.WriteLine("TextureCoordinates0: " + pUV1s.Length);
                WriteTabs(nLevel + 1);
                foreach (Vector2 uv in pUV1s) writer.Write(uv.x + " " + uv.y + " ");
                writer.WriteLine(" ");
            }

            if (pUV2s != null)
            {
                WriteTabs(nLevel + 1);
                writer.WriteLine("TextureCoordinates1: " + pUV2s.Length);
                WriteTabs(nLevel + 1);
                foreach (Vector2 uv in pUV2s) writer.Write(uv.x + " " + uv.y + " ");
                writer.WriteLine(" ");
            }
        }
        else
        {
            SkinnedMeshRenderer skinnedMeshRenderer = current.gameObject.GetComponent<SkinnedMeshRenderer>();
            if (skinnedMeshRenderer)
            {
                WriteTabs(nLevel + 1);
                writer.WriteLine("SkinnedMeshName: " + skinnedMeshRenderer.name);

                Mesh mesh = skinnedMeshRenderer.sharedMesh;

                Transform[] bones = skinnedMeshRenderer.bones;
                //BoneWeight[] Mesh.boneWeights; //정점마다, 각 정점은 최대 4개까지의 뼈대 영향을 받을 수 있음, Mesh.GetBoneWeights(List<BoneWeight> boneWeights); 
                //int BoneWeight.boneIndex0 ~ boneIndex3 float BoneWeight.weight0 ~ weight3
                //Matrix4x4[] Mesh.bindposes; //inverse of the transformation matrix of the bone, Mesh.GetBindposes(List<Matrix4x4> bindposes); 
                //int Mesh.blendShapeCount
                //float GetBlendShapeWeight(int index); 
                Vector3[] pVertices = mesh.vertices;
                Vector3[] pNormals = mesh.normals;
                Vector2[] pUV1s = mesh.uv;
                Vector2[] pUV2s = mesh.uv2;
                int[] pIndices = mesh.triangles;

                if (pVertices != null)
                {
                    WriteTabs(nLevel + 1);
                    writer.WriteLine("Vertices: " + pVertices.Length);
                    WriteTabs(nLevel + 1);
                    foreach (Vector3 v in pVertices) writer.Write(v.x + " " + v.y + " " + v.z + " ");
                    writer.WriteLine(" ");
                }

                if (pNormals != null)
                {
                    WriteTabs(nLevel + 1);
                    writer.WriteLine("Normals: " + pNormals.Length);
                    WriteTabs(nLevel + 1);
                    foreach (Vector3 n in pNormals) writer.Write(n.x + " " + n.y + " " + n.z + " ");
                    writer.WriteLine(" ");
                }

                if (pIndices != null)
                {
                    WriteTabs(nLevel + 1);
                    writer.WriteLine("Indices: " + pIndices.Length);
                    WriteTabs(nLevel + 1);
                    foreach (int i in pIndices) writer.Write(i + " ");
                    writer.WriteLine(" ");
                }

                if (pUV1s != null)
                {
                    WriteTabs(nLevel + 1);
                    writer.WriteLine("TextureCoordinates0: " + pUV1s.Length);
                    WriteTabs(nLevel + 1);
                    foreach (Vector2 uv in pUV1s) writer.Write(uv.x + " " + uv.y + " ");
                    writer.WriteLine(" ");
                }

                if (pUV2s != null)
                {
                    WriteTabs(nLevel + 1);
                    writer.WriteLine("TextureCoordinates1: " + pUV2s.Length);
                    WriteTabs(nLevel + 1);
                    foreach (Vector2 uv in pUV2s) writer.Write(uv.x + " " + uv.y + " ");
                    writer.WriteLine(" ");
                }
            }
        }

        MeshRenderer renderer = current.gameObject.GetComponent<MeshRenderer>();
        if (renderer)
        {
            Material material = renderer.material;
            if (material)
            {
                Shader shader = material.shader;
                Texture albedoTexture = material.GetTexture("_MainTex");
                if (albedoTexture) writer.WriteLine("AlbedoTextureName: " + albedoTexture.name);
                Texture emissionTexture = material.GetTexture("_EmissionMap");
                if (emissionTexture) writer.WriteLine("EmissionTextureName: " + emissionTexture.name);

                WriteTabs(nLevel + 1);
                writer.Write("AlbedoColor: ");
                writer.WriteLine(material.color.r + " " + material.color.g + " " + material.color.b + " " + material.color.a + " ");

                if (material.mainTexture) writer.WriteLine("MainTextureName: " + material.mainTexture.name);
            }
        }
    }
    void WriteTransformInfo(int nLevel, Transform child)
    {
        WriteFrameInfo(nLevel, child);

        WriteTabs(nLevel + 1);
        writer.WriteLine("Children: " + child.childCount);

        if (child.childCount > 0)
        {
            for (int k = 0; k < child.childCount; k++)
            {
                WriteTransformInfo(nLevel + 1, child.GetChild(k));
            }
        }

        WriteTabs(nLevel);
        writer.WriteLine("EndOfFrame");
    }
    void Start ()
    {
        writer = new StreamWriter("Model.txt");

        WriteTransformInfo(0, transform);
    
        print("Mesh Write Completed");

        writer.Flush();
        writer.Close();
    }
	 
	// Update is called once per frame
	void Update () {
	}
}

