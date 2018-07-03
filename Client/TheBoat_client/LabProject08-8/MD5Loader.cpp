#include "stdafx.h"
#include "MD5Loader.h"


bool LoadMD5Model(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, std::wstring filename, Model3D& MD5Model, std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC*>& shaderResourceViewArray, std::vector<std::wstring> texFileNameArray, CMesh*& pMesh)
{
	std::wifstream fileIn(filename.c_str()); // Open file 
	std::wstring checkString; // Stores the next string from our file 
	if (fileIn) // Check if the file was opened 
	{
		while (fileIn) // Loop until the end of the file is reached *
		{
			fileIn >> checkString; // Get next string from file 
			if (checkString == L"MD5Version") // Get MD5 version (this function supports version 10) 
			{ /*
			  fileIn >> checkString;
			  MessageBox(0, checkString.c_str(), //display message L"MD5Version", MB_OK);*/
			}
			else if (checkString == L"commandline")
			{
				std::getline(fileIn, checkString); // Ignore the rest of this line 
			}
			else if (checkString == L"numJoints")
			{
				fileIn >> MD5Model.numJoints; // Store number of joints 
			}
			else if (checkString == L"numMeshes")
			{
				fileIn >> MD5Model.numSubsets; // Store number of meshes or subsets which we will call them 
			}
			else if (checkString == L"joints")
			{
				Joint tempJoint; fileIn >> checkString; // Skip the "{" 
				for (int i = 0; i < MD5Model.numJoints; i++)
				{
					fileIn >> tempJoint.name; // Store joints name 
											  // Sometimes the names might contain spaces. If that is the case, we need to continue
											  // to read the name until we get to the closing " (quotation marks) 
					if (tempJoint.name[tempJoint.name.size() - 1] != '"')
					{
						wchar_t checkChar;
						bool jointNameFound = false;
						while (!jointNameFound)
						{
							checkChar = fileIn.get();
							if (checkChar == '"')
								jointNameFound = true;
							tempJoint.name += checkChar;
						}
					}
					fileIn >> tempJoint.parentID; // Store Parent joint's ID 
					fileIn >> checkString; // Skip the "(" // Store position of this joint (swap y and z axis if model was made in RH Coord Sys) 
					fileIn >> tempJoint.pos.x >> tempJoint.pos.z >> tempJoint.pos.y;
					fileIn >> checkString >> checkString; // Skip the ")" and "(" // Store orientation of this joint 
					fileIn >> tempJoint.orientation.x >> tempJoint.orientation.z >> tempJoint.orientation.y; // Remove the quotation marks from joints name 
					tempJoint.name.erase(0, 1);
					tempJoint.name.erase(tempJoint.name.size()-1, 1); 
					// Compute the w axis of the quaternion (The MD5 model uses a 3D vector to describe the
					// direction the bone is facing. However, we need to turn this into a quaternion, and the way 
					// quaternions work, is the xyz values describe the axis of rotation, while the w is a value 
					// between 0 and 1 which describes the angle of rotation) 
					float t = 1.0f - (tempJoint.orientation.x * tempJoint.orientation.x) - (tempJoint.orientation.y * tempJoint.orientation.y) - (tempJoint.orientation.z * tempJoint.orientation.z);
					if (t < 0.0f)
					{
						tempJoint.orientation.w = 0.0f;
					}
					else {
						tempJoint.orientation.w = -sqrtf(t);
					}
					std::getline(fileIn, checkString); // Skip rest of this line 
					MD5Model.joints.push_back(tempJoint); // Store the joint into this models joint vector 
				}
				fileIn >> checkString; // Skip the "}" 
			}
			else if (checkString == L"mesh")
			{
				ModelSubset subset;
				int numVerts, numTris, numWeights;
				fileIn >> checkString; // Skip the "{" 
				fileIn >> checkString;
				while (checkString != L"}") // Read until '}'
				{
					// In this lesson, for the sake of simplicity, we will assume a textures filename is givin here. 
					// Usually though, the name of a material (stored in a material library. Think back to the lesson on 
					// loading .obj files, where the material library was contained in the file .mtl) is givin. Let this 
					// be an exercise to load the material from a material library such as obj's .mtl file, instead of 
					// just the texture like we will do here. 
					if (checkString == L"shader") // Load the texture or material
					{
						std::wstring fileNamePath;
						fileIn >> fileNamePath;	// Get texture's filename 
												// Take spaces into account if filename or material name has a space in it 
						if (fileNamePath[fileNamePath.size() - 1] != '"')
						{
							wchar_t checkChar;
							bool fileNameFound = false;
							while (!fileNameFound)
							{
								checkChar = fileIn.get();
								if (checkChar == '"')
									fileNameFound = true;
								fileNamePath += checkChar;
							}
						} // Remove the quotation marks from texture path 
						fileNamePath.erase(0, 1);
						fileNamePath.erase(fileNamePath.size() - 1, 1); //check if this texture has already been loaded 
						bool alreadyLoaded = false;
						for (int i = 0; i < texFileNameArray.size(); ++i)
						{
							if (fileNamePath == texFileNameArray[i])
							{
								alreadyLoaded = true;
								subset.texArrayIndex = i;
							}
						} //if the texture is not already loaded, load it now 
						if (!alreadyLoaded)
						{
							//;
							//
							////hr = CreateShaderResourceView(md3dDevice, fileNamePath.c_str(), NULL, NULL, &tempMeshSRV, NULL);
							//if (SUCCEEDED(hr))
							//{
							//	texFileNameArray.push_back(fileNamePath.c_str());
							//	subset.texArrayIndex = shaderResourceViewArray.size();
							//	shaderResourceViewArray.push_back(tempMeshSRV);
							//}
							//else
							//{
							//	MessageBox(0, fileNamePath.c_str(), //display message 
							//		L"Could Not Open:",
							//		MB_OK);
							//	return false;
							//}
						}
						std::getline(fileIn, checkString); // Skip rest of this line 
					}
					else if (checkString == L"numverts")
					{
						fileIn >> numVerts; // Store number of vertices 
						std::getline(fileIn, checkString); // Skip rest of this line
						for (int i = 0; i < numVerts; i++)
						{
							Vertex1 tempVert;
							fileIn >> checkString // Skip "vert # (" 
								>> checkString
								>> checkString;
							fileIn >> tempVert.texCoord.x // Store tex coords 
								>> tempVert.texCoord.y; fileIn
								>> checkString; // Skip ")" 
							fileIn >> tempVert.StartWeight; // Index of first weight this vert will be weighted to 
							fileIn >> tempVert.WeightCount; // Number of weights for this vertex 
							std::getline(fileIn, checkString); // Skip rest of this line 
							subset.vertices.push_back(tempVert); // Push back this vertex into subsets vertex vector 
						}
					}
					else if (checkString == L"numtris")
					{
						fileIn >> numTris;
						subset.numTriangles = numTris;
						std::getline(fileIn, checkString); // Skip rest of this line
						for (int i = 0; i < numTris; i++) // Loop through each triangle
						{
							DWORD tempIndex;
							fileIn >> checkString; // Skip "tri" 
							fileIn >> checkString; // Skip tri counter 
							for (int k = 0; k < 3; k++) // Store the 3 indices 
							{
								fileIn >> tempIndex;
								subset.indices.push_back(tempIndex);
							}
							std::getline(fileIn, checkString); // Skip rest of this line
						}
					}
					else if (checkString == L"numweights")
					{
						fileIn >> numWeights;
						std::getline(fileIn, checkString);
						for (int i = 0; i < numWeights; i++)
						{
							Weight tempWeight;
							fileIn >> checkString >> checkString;
							fileIn >> tempWeight.jointID;
							fileIn >> tempWeight.bias;
							fileIn >> checkString;
							fileIn >> tempWeight.pos.x
								>> tempWeight.pos.z
								>> tempWeight.pos.y;
							std::getline(fileIn, checkString);
							subset.weights.push_back(tempWeight);
						}
					}
					else
						std::getline(fileIn, checkString);
					fileIn >> checkString;
				}
				for (int i = 0; i < subset.vertices.size(); ++i)
				{
					Vertex1 tempVert = subset.vertices[i];
					tempVert.pos = XMFLOAT3(0, 0, 0);
					for (int j = 0; j < tempVert.WeightCount; ++j)
					{
						Weight tempWeight = subset.weights[tempVert.StartWeight + j];
						Joint tempJoint = MD5Model.joints[tempWeight.jointID];
						XMVECTOR tempJointOrientation = XMVectorSet(tempJoint.orientation.x, tempJoint.orientation.y, tempJoint.orientation.z, tempJoint.orientation.w);
						XMVECTOR tempWeightPos = XMVectorSet(tempWeight.pos.x, tempWeight.pos.y, tempWeight.pos.z, 0.0f);
						XMVECTOR tempJointOrientationConjugate = XMVectorSet(-tempJoint.orientation.x, -tempJoint.orientation.y, -tempJoint.orientation.z, tempJoint.orientation.w);
						XMFLOAT3 rotatedPoint;
						XMStoreFloat3(&rotatedPoint, XMQuaternionMultiply(XMQuaternionMultiply(tempJointOrientation, tempWeightPos), tempJointOrientationConjugate));
						tempVert.pos.x += (tempJoint.pos.x + rotatedPoint.x) * tempWeight.bias;
						tempVert.pos.y += (tempJoint.pos.y + rotatedPoint.y) * tempWeight.bias;
						tempVert.pos.z += (tempJoint.pos.z + rotatedPoint.z) * tempWeight.bias;
					}
					subset.positions.push_back(tempVert.pos);
				}
				for (int i = 0; i < subset.vertices.size(); i++)
				{
					subset.vertices[i].pos = subset.positions[i];
				}


				std::vector<XMFLOAT3> tempNormal;
				XMFLOAT3 unnormalized = XMFLOAT3(0.0f, 0.0f, 0.0f);
				float vecX, vecY, vecZ;
				XMVECTOR edge1 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
				XMVECTOR edge2 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
				for (int i = 0; i < subset.numTriangles; ++i)
				{
					//Get the vector describing one edge of our triangle (edge 0,2) 
					vecX = subset.vertices[subset.indices[(i * 3)]].pos.x - subset.vertices[subset.indices[(i * 3) + 2]].pos.x;
					vecY = subset.vertices[subset.indices[(i * 3)]].pos.y - subset.vertices[subset.indices[(i * 3) + 2]].pos.y;
					vecZ = subset.vertices[subset.indices[(i * 3)]].pos.z - subset.vertices[subset.indices[(i * 3) + 2]].pos.z;
					edge1 = XMVectorSet(vecX, vecY, vecZ, 0.0f); //Create our first edge //Get the vector describing another edge of our triangle (edge 2,1) 
					vecX = subset.vertices[subset.indices[(i * 3) + 2]].pos.x - subset.vertices[subset.indices[(i * 3) + 1]].pos.x;
					vecY = subset.vertices[subset.indices[(i * 3) + 2]].pos.y - subset.vertices[subset.indices[(i * 3) + 1]].pos.y;
					vecZ = subset.vertices[subset.indices[(i * 3) + 2]].pos.z - subset.vertices[subset.indices[(i * 3) + 1]].pos.z;
					edge2 = XMVectorSet(vecX, vecY, vecZ, 0.0f); //Create our second edge //Cross multiply the two edge vectors to get the un-normalized face normal 
					XMStoreFloat3(&unnormalized, XMVector3Cross(edge1, edge2)); tempNormal.push_back(unnormalized);
				} //Compute vertex normals (normal Averaging)
				XMVECTOR normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
				int facesUsing = 0;
				float tX, tY, tZ; //temp axis variables //Go through each vertex 
				for (int i = 0; i < subset.vertices.size(); ++i)
				{
					//Check which triangles use this vertex 
					for (int j = 0; j < subset.numTriangles; ++j)
					{
						if (subset.indices[j * 3] == i || subset.indices[(j * 3) + 1] == i || subset.indices[(j * 3) + 2] == i)
						{
							tX = XMVectorGetX(normalSum) + tempNormal[j].x;
							tY = XMVectorGetY(normalSum) + tempNormal[j].y;
							tZ = XMVectorGetZ(normalSum) + tempNormal[j].z;
							normalSum = XMVectorSet(tX, tY, tZ, 0.0f); //If a face is using the vertex, add the unormalized face normal to the normalSum 
							facesUsing++;
						}
					} //Get the actual normal by dividing the normalSum by the number of faces sharing the vertex 
					normalSum = normalSum / facesUsing; //Normalize the normalSum vector 
					normalSum = XMVector3Normalize(normalSum); //Store the normal and tangent in our current vertex 
					subset.vertices[i].normal.x = -XMVectorGetX(normalSum);
					subset.vertices[i].normal.y = -XMVectorGetY(normalSum);
					subset.vertices[i].normal.z = -XMVectorGetZ(normalSum); //Clear normalSum, facesUsing for next vertex 
					normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f); facesUsing = 0;
				} // Create index buffer 


				UINT *pnIndices = NULL;
				int nIndices = 0;
				int nVertices = subset.vertices.size();
				XMFLOAT3 *pxmf3Positions = NULL;
				XMFLOAT3 *pxmf3Normals = NULL;
				XMFLOAT2 *pxmf2texCoord = NULL;
				
				pxmf3Positions = new XMFLOAT3[nVertices];
				for (int i = 0; i < nVertices; i++)
				{
					pxmf3Positions[i].x = subset.vertices[i].pos.x;
					pxmf3Positions[i].y = subset.vertices[i].pos.y;
					pxmf3Positions[i].z = -1 * subset.vertices[i].pos.z;
				}
				pxmf3Normals = new XMFLOAT3[nVertices];
				for (int i = 0; i < nVertices; i++)
				{
					pxmf3Normals[i].x = subset.vertices[i].normal.x;
					pxmf3Normals[i].y = subset.vertices[i].normal.y;
					pxmf3Normals[i].z = subset.vertices[i].normal.z;
				}
				pxmf2texCoord = new XMFLOAT2[nVertices];
				for (int i = 0; i < nVertices; i++)
				{

					pxmf2texCoord[i].x = subset.vertices[i].texCoord.x;
					pxmf2texCoord[i].y = subset.vertices[i].texCoord.y;

				}
				pnIndices = new UINT[subset.indices.size()];
				for (int i = 0; i < subset.indices.size(); i++)
				{

					pnIndices[i] = subset.indices[i];
				}
				XMFLOAT3 *aaa = NULL;
				aaa = new XMFLOAT3[subset.indices.size()];
				for (int i = 0; i < subset.indices.size(); i++) {
					aaa[i] = subset.vertices[subset.indices[i]].pos;
				}
				//pMesh = new CMeshTextured(pd3dDevice, pd3dCommandList, subset.indices.size(), aaa, pxmf2texCoord, subset.indices.size(), pnIndices);

				//pMesh = new CMeshTextured(pd3dDevice, pd3dCommandList, numVerts, pxmf3Positions, pxmf2texCoord, subset.indices.size(), pnIndices);
				pMesh = new CMeshIlluminatedTextured(pd3dDevice, pd3dCommandList, numVerts, pxmf3Positions, pxmf3Normals, pxmf2texCoord, subset.indices.size(), pnIndices);
				
				//pMesh = new CMeshTextured(pd3dDevice, pd3dCommandList, numVerts, pxmf3Positions, pxmf2texCoord, subset.indices.size(), pnIndices);
				
				MD5Model.subsets.push_back(subset);
			}
		}
	}
	else
	{
		//SwapChain->SetFullscreenState(false, NULL); // Make sure we are out of fullscreen // create message 
		std::wstring message = L"Could not open: ";
		message += filename;
		MessageBox(0, message.c_str(), // display message 
			L"Error", MB_OK);
		return false;
	}
 	return true;
}

bool LoadMD5Anim(std::wstring filename, Model3D & MD5Model) {
	ModelAnimation tempAnim; // Temp animation to later store in our model's animation array 
	std::wifstream fileIn(filename.c_str()); // Open file 
	std::wstring checkString; // Stores the next string from our file 
	if (fileIn) // Check if the file was opened 
	{
		while (fileIn) // Loop until the end of the file is reached 
		{
			fileIn >> checkString; // Get next string from file 
			if (checkString == L"MD5Version") // Get MD5 version (this function supports version 10) 
			{
				fileIn >> checkString; /*MessageBox(0, checkString.c_str(), //display message L"MD5Version", MB_OK);*/
			}
			else if (checkString == L"commandline")
			{
				std::getline(fileIn, checkString); // Ignore the rest of this line 

			}
			else if (checkString == L"numFrames")
			{
				fileIn >> tempAnim.numFrames; // Store number of frames in this animation
			}
			else if (checkString == L"numJoints")
			{
				fileIn >> tempAnim.numJoints; // Store number of joints (must match .md5mesh) 
			}
			else if (checkString == L"frameRate")
			{
				fileIn >> tempAnim.frameRate;
			}
			else if (checkString == L"numAnimatedComponents")
			{
				fileIn >> tempAnim.numAnimatedComponents;
			}
			else if (checkString == L"hierarchy")
			{
				fileIn >> checkString;
				for (int i = 0; i < tempAnim.numJoints; i++)
				{
					AnimJointInfo tempJoint;
					fileIn >> tempJoint.name;
					if (tempJoint.name[tempJoint.name.size() - 1] != '"')
					{
						wchar_t checkChar;
						bool jointNameFound = false;
						while (!jointNameFound)
						{
							checkChar = fileIn.get();
							if (checkChar == '"')
								jointNameFound = true;
							tempJoint.name += checkChar;
						}
					}
					tempJoint.name.erase(0, 1);
					tempJoint.name.erase(tempJoint.name.size() - 1, 1);
					fileIn >> tempJoint.parentID;
					fileIn >> tempJoint.flags;
					fileIn >> tempJoint.startIndex;
					bool jointMatchFound = false;
					for (int k = 0; k < MD5Model.numJoints; k++)
					{
						if (MD5Model.joints[k].name == tempJoint.name)
						{
							if (MD5Model.joints[k].parentID == tempJoint.parentID)
							{
								jointMatchFound = true;
								tempAnim.jointInfo.push_back(tempJoint);
							}
						}
					}
					if (!jointMatchFound)
						return false;
					std::getline(fileIn, checkString);
				}
			}
			else if (checkString == L"bounds") // Load in the AABB for each animation
			{
				fileIn >> checkString; // Skip opening bracket "{" 
				for (int i = 0; i < tempAnim.numFrames; i++)
				{
					BoundingBox1 tempBB; fileIn >> checkString; // Skip "("
					fileIn >> tempBB.min.x >> tempBB.min.z >> tempBB.min.y;
					fileIn >> checkString >> checkString; // Skip ") (" 
					fileIn >> tempBB.max.x >> tempBB.max.z >> tempBB.max.y;
					fileIn >> checkString; // Skip ")"
					tempAnim.frameBounds.push_back(tempBB);
				}
			}
			else if (checkString == L"baseframe") // This is the default position for the animation 
			{ // All frames will build their skeletons off this 
				fileIn >> checkString; // Skip opening bracket "{"
				for (int i = 0; i < tempAnim.numJoints; i++)
				{
					Joint tempBFJ; fileIn >> checkString; // Skip "(" 
					fileIn >> tempBFJ.pos.x >> tempBFJ.pos.z >> tempBFJ.pos.y;
					fileIn >> checkString >> checkString; // Skip ") (" 
					fileIn >> tempBFJ.orientation.x >> tempBFJ.orientation.z >> tempBFJ.orientation.y;
					fileIn >> checkString; // Skip ")" 
					tempAnim.baseFrameJoints.push_back(tempBFJ);
				}
			}
			else if (checkString == L"frame") // Load in each frames skeleton (the parts of each joint that changed from the base frame)
			{
				FrameData tempFrame;
				fileIn >> tempFrame.frameID; // Get the frame ID 
				fileIn >> checkString; // Skip opening bracket "{"
				for (int i = 0; i < tempAnim.numAnimatedComponents; i++)
				{
					float tempData; fileIn >> tempData; // Get the data 
					tempFrame.frameData.push_back(tempData);
				}
				tempAnim.frameData.push_back(tempFrame);
				std::vector<Joint> tempSkeleton;
				for (int i = 0; i < tempAnim.jointInfo.size(); i++) {
					int k = 0;
					Joint tempFrameJoint = tempAnim.baseFrameJoints[i];
					tempFrameJoint.parentID = tempAnim.jointInfo[i].parentID;
					if (tempAnim.jointInfo[i].flags & 1)
						tempFrameJoint.pos.x = tempFrame.frameData[tempAnim.jointInfo[i].startIndex + k++];
					if (tempAnim.jointInfo[i].flags & 2)
						tempFrameJoint.pos.z = tempFrame.frameData[tempAnim.jointInfo[i].startIndex + k++];
					if (tempAnim.jointInfo[i].flags & 4)
						tempFrameJoint.pos.y = tempFrame.frameData[tempAnim.jointInfo[i].startIndex + k++];
					if (tempAnim.jointInfo[i].flags & 8)
						tempFrameJoint.orientation.x = tempFrame.frameData[tempAnim.jointInfo[i].startIndex + k++];
					if (tempAnim.jointInfo[i].flags & 16)
						tempFrameJoint.orientation.z = tempFrame.frameData[tempAnim.jointInfo[i].startIndex + k++];
					if (tempAnim.jointInfo[i].flags & 32)
						tempFrameJoint.orientation.y = tempFrame.frameData[tempAnim.jointInfo[i].startIndex + k++];
					float t = 1.0f - (tempFrameJoint.orientation.x * tempFrameJoint.orientation.x)
						- (tempFrameJoint.orientation.y * tempFrameJoint.orientation.y)
						- (tempFrameJoint.orientation.z * tempFrameJoint.orientation.z);
					if (t < 0.0f) { tempFrameJoint.orientation.w = 0.0f; }
					else { tempFrameJoint.orientation.w = -sqrtf(t); }
					if (tempFrameJoint.parentID >= 0) {
						Joint parentJoint = tempSkeleton[tempFrameJoint.parentID];
						XMVECTOR parentJointOrientation = XMVectorSet(parentJoint.orientation.x, parentJoint.orientation.y, parentJoint.orientation.z, parentJoint.orientation.w);
						XMVECTOR tempJointPos = XMVectorSet(tempFrameJoint.pos.x, tempFrameJoint.pos.y, tempFrameJoint.pos.z, 0.0f);
						XMVECTOR parentOrientationConjugate = XMVectorSet(-parentJoint.orientation.x, -parentJoint.orientation.y, -parentJoint.orientation.z, parentJoint.orientation.w);
						XMFLOAT3 rotatedPos;
						XMStoreFloat3(&rotatedPos, XMQuaternionMultiply(XMQuaternionMultiply(parentJointOrientation, tempJointPos), parentOrientationConjugate));
						tempFrameJoint.pos.x = rotatedPos.x + parentJoint.pos.x;
						tempFrameJoint.pos.y = rotatedPos.y + parentJoint.pos.y;
						tempFrameJoint.pos.z = rotatedPos.z + parentJoint.pos.z; // Currently the joint is oriented in its parent joints space, we now need to orient it in // model space by multiplying the two orientations together (parentOrientation * childOrientation) <- In that order 
						XMVECTOR tempJointOrient = XMVectorSet(tempFrameJoint.orientation.x, tempFrameJoint.orientation.y, tempFrameJoint.orientation.z, tempFrameJoint.orientation.w);
						tempJointOrient = XMQuaternionMultiply(parentJointOrientation, tempJointOrient); // Normalize the orienation quaternion 
						tempJointOrient = XMQuaternionNormalize(tempJointOrient);
						XMStoreFloat4(&tempFrameJoint.orientation, tempJointOrient);
					} // Store the joint into our temporary frame skeleton 
					tempSkeleton.push_back(tempFrameJoint);
				} // Push back our newly created frame skeleton into the animation's frameSkeleton array 
				tempAnim.frameSkeleton.push_back(tempSkeleton);
				fileIn >> checkString; // Skip closing bracket "}"
			}
		} // Calculate and store some usefull animation data 
		tempAnim.frameTime = 1.0f / tempAnim.frameRate; // Set the time per frame 
		tempAnim.totalAnimTime = tempAnim.numFrames * tempAnim.frameTime; // Set the total time the animation takes 
		tempAnim.currAnimTime = 0.0f; // Set the current time to zero
		
		MD5Model.animations.push_back(tempAnim); // Push back the animation into our model object \

	}
	else // If the file was not loaded
	{
		//SwapChain->SetFullscreenState(false, NULL); // Make sure we are out of fullscreen // create message 
		std::wstring message = L"Could not open: ";
		message += filename;
		MessageBox(0, message.c_str(), // display message
			L"Error", MB_OK);
		return false;
	}
	return true;
}

void UpdateMD5Model(Model3D & MD5Model, float deltaTime, int animation, CMesh*& pMesh, int meshnum) {
	//printf("%f", MD5Model.animations[animation].currAnimTime);
	MD5Model.animations[animation].currAnimTime += deltaTime; // Update the current animation time 
	if (MD5Model.animations[animation].currAnimTime > MD5Model.animations[animation].totalAnimTime)
		MD5Model.animations[animation].currAnimTime = 0.0f; // Which frame are we on 
	float currentFrame = MD5Model.animations[animation].currAnimTime * MD5Model.animations[animation].frameRate;
	int frame0 = floorf(currentFrame);
	int frame1 = frame0 + 1; // Make sure we don't go over the number of frames 
	if (frame0 == MD5Model.animations[animation].numFrames - 1)
		frame1 = 0;
	float interpolation = currentFrame - frame0; // Get the remainder (in time) between frame0 and frame1 to use as interpolation factor 
	std::vector<Joint> interpolatedSkeleton; // Create a frame skeleton to store the interpolated skeletons in // Compute the interpolated skeleton
	for (int i = 0; i < MD5Model.animations[animation].numJoints; i++)
	{
		Joint tempJoint;
		Joint joint0 = MD5Model.animations[animation].frameSkeleton[frame0][i]; // Get the i'th joint of frame0's skeleton
		Joint joint1 = MD5Model.animations[animation].frameSkeleton[frame1][i]; // Get the i'th joint of frame1's skeleton 
		tempJoint.parentID = joint0.parentID; // Set the tempJoints parent id // Turn the two quaternions into XMVECTORs for easy computations
		XMVECTOR joint0Orient = XMVectorSet(joint0.orientation.x, joint0.orientation.y, joint0.orientation.z, joint0.orientation.w);
		XMVECTOR joint1Orient = XMVectorSet(joint1.orientation.x, joint1.orientation.y, joint1.orientation.z, joint1.orientation.w); // Interpolate positions
		tempJoint.pos.x = joint0.pos.x + (interpolation * (joint1.pos.x - joint0.pos.x));
		tempJoint.pos.y = joint0.pos.y + (interpolation * (joint1.pos.y - joint0.pos.y));
		tempJoint.pos.z = joint0.pos.z + (interpolation * (joint1.pos.z - joint0.pos.z)); // Interpolate orientations using spherical interpolation (Slerp)
		XMStoreFloat4(&tempJoint.orientation, XMQuaternionSlerp(joint0Orient, joint1Orient, interpolation));
		interpolatedSkeleton.push_back(tempJoint); // Push the joint back into our interpolated skeleton
	}
	/*for (int k = 0; k < MD5Model.numSubsets; k++), int meshnum
	{*/
		for (int i = 0; i < MD5Model.subsets[meshnum].vertices.size(); ++i)
		{
			Vertex1 tempVert = MD5Model.subsets[meshnum].vertices[i];
			tempVert.pos = XMFLOAT3(0, 0, 0); // Make sure the vertex's pos is cleared first
			tempVert.normal = XMFLOAT3(0, 0, 0); // Clear vertices normal // Sum up the joints and weights information to get vertex's position and normal
			for (int j = 0; j < tempVert.WeightCount; ++j)
			{
				Weight tempWeight = MD5Model.subsets[meshnum].weights[tempVert.StartWeight + j];
				Joint tempJoint = interpolatedSkeleton[tempWeight.jointID]; // Convert joint orientation and weight pos to vectors for easier computation
				XMVECTOR tempJointOrientation = XMVectorSet(tempJoint.orientation.x, tempJoint.orientation.y, tempJoint.orientation.z, tempJoint.orientation.w);
				XMVECTOR tempWeightPos = XMVectorSet(tempWeight.pos.x, tempWeight.pos.y, tempWeight.pos.z, 0.0f); // We will need to use the conjugate of the joint orientation quaternion
				XMVECTOR tempJointOrientationConjugate = XMQuaternionInverse(tempJointOrientation); // Calculate vertex position (in joint space, eg. rotate the point around (0,0,0)) for this weight using the joint orientation quaternion and its conjugate // We can rotate a point using a quaternion with the equation "rotatedPoint = quaternion * point * quaternionConjugate" 
				XMFLOAT3 rotatedPoint;
				XMStoreFloat3(&rotatedPoint, XMQuaternionMultiply(XMQuaternionMultiply(tempJointOrientation, tempWeightPos), tempJointOrientationConjugate)); // Now move the verices position from joint space (0,0,0) to the joints position in world space, taking the weights bias into account
				tempVert.pos.x += (tempJoint.pos.x + rotatedPoint.x) * tempWeight.bias;
				tempVert.pos.y += (tempJoint.pos.y + rotatedPoint.y) * tempWeight.bias;
				tempVert.pos.z += (tempJoint.pos.z + rotatedPoint.z) * tempWeight.bias; // Compute the normals for this frames skeleton using the weight normals from before // We can comput the normals the same way we compute the vertices position, only we don't have to translate them (just rotate) 
				XMVECTOR tempWeightNormal = XMVectorSet(tempWeight.normal.x, tempWeight.normal.y, tempWeight.normal.z, 0.0f); // Rotate the normal 
				XMStoreFloat3(&rotatedPoint, XMQuaternionMultiply(XMQuaternionMultiply(tempJointOrientation, tempWeightNormal), tempJointOrientationConjugate)); // Add to vertices normal and ake weight bias into account 
				tempVert.normal.x -= rotatedPoint.x * tempWeight.bias;
				tempVert.normal.y -= rotatedPoint.y * tempWeight.bias;
				tempVert.normal.z -= rotatedPoint.z * tempWeight.bias;
			}
			MD5Model.subsets[meshnum].positions[i] = tempVert.pos; // Store the vertices position in the position vector instead of straight into the vertex vector
			MD5Model.subsets[meshnum].vertices[i].normal = tempVert.normal; // Store the vertices normal
			XMStoreFloat3(&MD5Model.subsets[meshnum].vertices[i].normal, XMVector3Normalize(XMLoadFloat3(&MD5Model.subsets[meshnum].vertices[i].normal)));
		} 
		for (int i = 0; i < MD5Model.subsets[meshnum].vertices.size(); i++)
		{
			MD5Model.subsets[meshnum].vertices[i].pos = MD5Model.subsets[meshnum].positions[i];
			MD5Model.subsets[meshnum].vertices[i].pos.x = MD5Model.subsets[meshnum].positions[i].x;
			MD5Model.subsets[meshnum].vertices[i].pos.y = MD5Model.subsets[meshnum].positions[i].y;
			MD5Model.subsets[meshnum].vertices[i].pos.z = -1 * MD5Model.subsets[meshnum].positions[i].z;
		}
		pMesh->A->CopyData(0, MD5Model.subsets[meshnum].vertices[0], (MD5Model.subsets[meshnum].vertices.size()));

	//}
}