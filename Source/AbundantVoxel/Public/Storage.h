#pragma once

#include "CoreMinimal.h"
#include "Common.h"

using NodeIndex = uint32;

struct FNodeContext
{
	FNode Node;
	NodeIndex Index;
	NodeIndex NodeIndexUp;
	union
	{
		NodeIndex NodeIndexDown;
		uint32 Data;
	};
	uint8 ChildIndexUp;
	union
	{
		uint8 ChildIndex;
		uint8 DataIndex;
	};
	uint8 Depth;

	FNodeContext NodeDown()const
	{
		FNodeContext result = *this;
		result.Depth = Depth - 1;
		result.NodeIndexUp = Index;
		result.Index = NodeIndexDown;
		result.ChildIndexUp = ChildIndex;
		return result;
	}
};

class FDAGNode
{
public:
	FDAGNode();
	~FDAGNode();

	NodeIndex InsertInternal(const FNode& data);
	NodeIndex InsertData(const FNode& data);

	NodeIndex UpdataInternalChild(FNodeContext& context);
	NodeIndex UpdataDataChild(FNodeContext& context);

	const FNode& GetNode(const NodeIndex& index)const;

	FNode RootNode;

private:
	void SubtractInternalRef(NodeIndex index);
	void SubtractDataRef(NodeIndex index);

	TArray<FNode> NodePool;
	TQueue<NodeIndex> FreeIndex;
	TMap<FNode::Node, NodeIndex> InternalMap;
	TMap<FNode::Node, NodeIndex> DataMap;
};

class ABUNDANTVOXEL_API FVolume
{
public:
	FVolume();
	void SetVoxelData(const FIntVector& vector, uint32 data);
	NodeIndex RecursionSetVoxelData(const FIntVector& vector, const FNodeContext& context, const uint32& data);
	uint32 GetVoxelData(const FIntVector& vector)const;
private:
	const static uint8 MaxDepth = 15;
	FDAGNode DAG;
};
