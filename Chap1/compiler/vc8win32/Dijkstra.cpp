void Dijkstra(int SphereNumber, int start, int finish)
{
	for (int j = 0; j < 40; j++)
		WayPoints2[SphereNumber][j] = INT_MAX;
	int distance[GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY], count, index, i, u, m = start + 1;
	bool visited[GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY];
	for (i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
	{
		distance[i] = INT_MAX;
		visited[i] = false;
	}
	distance[start] = 0;
	int countOfPoints = 0;
	for (count = 0; count < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - 1; count++)
	{
		int min = INT_MAX;
		for (i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
		if (!visited[i] && distance[i] <= min)
		{
			min = distance[i];
			index = i;
		}
		u = index;
		visited[u] = true;
		for (i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
		if (!visited[i] && (MatrixSmezhnost[u][i] == 1) && distance[u] != INT_MAX &&
			distance[u] + MatrixSmezhnost[u][i] < distance[i])
		{
			distance[i] = distance[u] + MatrixSmezhnost[u][i];
			PointsOfTheWay[SphereNumber][countOfPoints][0] = u;
			PointsOfTheWay[SphereNumber][countOfPoints][1] = i;
			countOfPoints++;
		}
	}
	int PointIndex = 0;
	int PrevPoint = 100000;
	for (int i = 100; i > 0; i--)
	{
		if (PointsOfTheWay[SphereNumber][i][1] == finish)
		{
			WayPoints2[SphereNumber][PointIndex] = finish;
			PointIndex++;
			PrevPoint = PointsOfTheWay[SphereNumber][i][0];
			WayPoints2[SphereNumber][PointIndex] = PrevPoint;
			PointIndex++;
		}
		else
		if (PointsOfTheWay[SphereNumber][i][1] == PrevPoint)
		{
			PrevPoint = PointsOfTheWay[SphereNumber][i][0];
			WayPoints2[SphereNumber][PointIndex] = PrevPoint;
			if (PrevPoint == start)
				break;
			else
				PointIndex++;
		}
	}
	WayPoints2[SphereNumber][PointIndex] = start;
	masRevers(WayPoints2, SphereNumber, PointIndex);
}