#pragma once
#include <cstdlib>
#include <vector>
#include <bx/math.h>

 


int GetIdx(int width, int row, int col)
{
	return width * row + col;
}

bool AddPoint(bx::Vec3 CandiPoint, int width,int height, int sizeX, int sizeY, float cellsize, float radius, std::vector<bx::Vec3>& Points, std::vector<int>& grid)
{
	bool pointvalid = true;

	if (CandiPoint.x > 0 && CandiPoint.x < sizeX && CandiPoint.y > 0 && CandiPoint.y < sizeY)
	{
		int gridX = (int)(CandiPoint.x / cellsize);
		int gridY = (int)(CandiPoint.y / cellsize);
		int gridIdx = GetIdx(width, gridX, gridY);

		int searchStartX = bx::max(0, gridX - 4);
		int searchEndX = bx::min(gridX+4, width-1);
		int searchStartY = bx::max(0, gridY - 4);
		int searchEndY = bx::min(gridY + 4, height - 1);

		
		float CandiScaleRadius = CandiPoint.z * radius;

		for (size_t i = searchStartX; i <= searchEndX; i++)
		{
			for (size_t j = searchStartY; j <= searchEndY; j++)
			{
				int pointidx = grid[GetIdx(width, i, j)]-1;
				if (pointidx > -1)
				{
					float dist = bx::distanceSq(CandiPoint, Points[pointidx]);

					float scaleradius = bx::max(CandiScaleRadius, Points[pointidx].z * radius);
					
					if (dist < scaleradius * scaleradius)
					{
						return false;
						pointvalid = false;
					}
				}
			}

		}
		return true;
	}
	else
	{
		pointvalid = false;
	}
	return false;
}

std::vector<bx::Vec3> buildmap(int seed, int sizeX, int sizeY)
{

	//trees - poisson
	float radius = 8;
	float cellsize = radius / bx::sqrt(2.0);
	int width = bx::ceil(sizeX / cellsize);
	int height = bx::ceil(sizeY / cellsize);
	std::vector<int> grid(width * height);
	std::vector<bx::Vec3> Points;
	std::vector<bx::Vec3> SpawnPoints;

	SpawnPoints.push_back(bx::Vec3(sizeX / 2, sizeY / 2, 0));

	std::srand(102);

	while (SpawnPoints.size() > 0)
	{
		int SpawnIdx = std::rand() % SpawnPoints.size();


		
		for (size_t i = 0; i < 12; i++)
		{
			float angle = (std::rand() / (float)RAND_MAX) * bx::kPi2;
			bx::Vec3 dir(bx::sin(angle), bx::cos(angle), 0);

			float distance = radius + (std::rand() / (float)RAND_MAX) * radius;
			dir.x = dir.x * distance;
			dir.y = dir.y * distance;

			bx::Vec3 CandiPoint = bx::add(SpawnPoints[SpawnIdx], dir);

			//try add CandiPoint
			if (AddPoint(CandiPoint, width, height, sizeX, sizeY, cellsize, radius, Points, grid))
			{
				bool Pointadded = true;
				SpawnPoints.push_back(CandiPoint);

				float scale = 1 + (std::rand() / (float)RAND_MAX)/4;
				CandiPoint.z = scale;

				Points.push_back(CandiPoint);

				int gridx = (int)(CandiPoint.x / cellsize);
				int gridy = (int)(CandiPoint.y / cellsize);
				int grididx = GetIdx(width, gridx, gridy);
				grid[grididx] = Points.size();
			}
		}
		SpawnPoints.erase(SpawnPoints.begin() + SpawnIdx);

	}

	//for (size_t i = 0; i < Points.size(); i++)
	//{
	//	Points[i].x -= (sizeX / 2);
	//	Points[i].y -= (sizeY / 2);
	//}
	return Points;
}