#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

using namespace std;

enum class FACETYPE
{
	TRIANGLE,
	QUAD
};

struct Index
{
	int vertex;
	int uv;
	int normal;
};

class Object
{
public:
	vector<float> positions;
	vector<float> normals;
	vector<float> texcoords;
	FACETYPE faceType = FACETYPE::TRIANGLE;

	void draw(){
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, vertex_cnt);
	}

	Object(const string& filename)
	{
		cout << filename << "\n";
		vector<unsigned int> vertexIndices, uvIndices, normalIndices;
		vector<glm::vec3> tempVertices;
		vector<glm::vec3> tempNormals;
		vector<glm::vec2> tempTexcoords;

		ifstream fs(filename);
		if (!fs.is_open())
		{
			cout << "Can't open obj file!\n";
			return;
		}

		// Read until eof
		while (!fs.eof())
		{
			string lineHead;
			// Read first part of current line
			fs >> lineHead;

			// This line represents a vertex coordinate
			if (lineHead == "v")
			{
				// Read vertex coordinate
				float x, y, z;
				fs >> x >> y >> z;
				tempVertices.push_back(glm::vec3(x, y, z));
			}
			// This line represents a normal
			else if (lineHead == "vn")
			{
				// Read normal
				float x, y, z;
				fs >> x >> y >> z;
				tempNormals.push_back(glm::vec3(x, y, z));
			}
			// This line represents a texture coordinate
			else if (lineHead == "vt")
			{
				// Read texture coordinate
				float u, v;
				fs >> u >> v;
				tempTexcoords.push_back(glm::vec2(u, v));
			}
			// This line represents a face
			else if (lineHead == "f")
			{
				// Read face
				string line;
				// Skip the first space
				getline(fs >> std::ws, line);
				// Mark end of line
				line += " end";
				// Use stringstream to split space
				stringstream ss;
				ss << line;
				string temp;
				Index indices[4];

				int numIndices = 0;
				// Keep spliting indices until reach "end of line"
				while (ss >> temp)
				{
					if (temp == "end")
					{
						break;
					}
					indices[numIndices] = splitIndex(temp);
					numIndices++;
				}
				// Check face type
				if (numIndices == 4)
					faceType = FACETYPE::QUAD;


				if (faceType == FACETYPE::QUAD)
				{
					for (int i = 0; i < 3; i += 2)
					{
						vertexIndices.push_back(indices[i].vertex);
						vertexIndices.push_back(indices[i + 1].vertex);
						vertexIndices.push_back(indices[(i + 2) % 4].vertex);

						uvIndices.push_back(indices[i].uv);
						uvIndices.push_back(indices[i + 1].uv);
						uvIndices.push_back(indices[(i + 2) % 4].uv);

						normalIndices.push_back(indices[i].normal);
						normalIndices.push_back(indices[i + 1].normal);
						normalIndices.push_back(indices[(i + 2) % 4].normal);
					}
				}
				else
				{
					vertexIndices.push_back(indices[0].vertex);
					vertexIndices.push_back(indices[1].vertex);
					vertexIndices.push_back(indices[2].vertex);

					uvIndices.push_back(indices[0].uv);
					uvIndices.push_back(indices[1].uv);
					uvIndices.push_back(indices[2].uv);

					normalIndices.push_back(indices[0].normal);
					normalIndices.push_back(indices[1].normal);
					normalIndices.push_back(indices[2].normal);
				}
			}
		}

		// Store information of object
		for (unsigned int i = 0; i < vertexIndices.size(); i++)
		{
			unsigned int index = vertexIndices[i];
			glm::vec3 vertex = tempVertices[index - 1];
			positions.push_back(vertex.x);
			positions.push_back(vertex.y);
			positions.push_back(vertex.z);

			index = uvIndices[i];
			if (index != -1)
			{
				glm::vec2 uv = tempTexcoords[index - 1];
				texcoords.push_back(uv.x);
				texcoords.push_back(uv.y);
			}
			else
			{
				texcoords.push_back(0);
				texcoords.push_back(0);
			}

			index = normalIndices[i];
			glm::vec3 normal = tempNormals[index - 1];
			normals.push_back(normal.x);
			normals.push_back(normal.y);
			normals.push_back(normal.z);
		}

		set_VAO();
	}

private:
	unsigned int VAO;
	int vertex_cnt;
	void set_VAO(){
		unsigned int VBO[3];
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glGenBuffers(3, VBO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT) * (positions.size()), &(positions[0]), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 3, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT) * (normals.size()), &(normals[0]), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 3, 0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT) * (texcoords.size()), &(texcoords[0]), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 2, 0);
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);

		vertex_cnt = positions.size();
		positions.clear();
		texcoords.clear();
		normals.clear();
	}
	Index splitIndex(string& str)
	{
		vector<int> results;
		int start = 0;
		int pos = str.find('/');
		while (pos != string::npos)
		{
			int length = pos - start;
			string sub = str.substr(start, length);
			results.push_back(sub == "" ? -1 : stol(sub));
			start = pos + 1;
			pos = str.find('/', start);
		}
		results.push_back(stol(str.substr(start)));

		return Index{ results[0], results[1], results[2] };
	}
};