
#include "Storage.h"

#define GET_BIT(x,y) ((x)>>(y)&1)
#define SET_BIT_ONE(x,y) (x|=(1<<y))
#define SET_BIT_ZERO(x,y) (x&=~(1<<y))

PRAGMA_DISABLE_OPTIMIZATION

FORCEINLINE uint8 GetChildIndex(const FIntVector& vector, uint32 level)
{
	uint8 tempx = GET_BIT(vector.X, level);
	uint8 tempy = GET_BIT(vector.Y, level) << 1;
	uint8 tempz = GET_BIT(vector.Z, level) << 2;
	return tempx | tempy | tempz;
}

FDAGNode::FDAGNode()
{
	NodePool.Reset(0xffff);
}

FDAGNode::~FDAGNode()
{
}

NodeIndex FDAGNode::InsertInternal(const FNode& data)
{
	NodeIndex result;
	NodeIndex* findValue = InternalMap.Find(data.Data);
	if (findValue == nullptr)
	{
		if (FreeIndex.IsEmpty())
		{
			result = NodePool.Num();
			NodePool.Emplace(data);
			InternalMap.Add(data.Data, result);
			return result;
		}
		else
		{
			FreeIndex.Dequeue(result);
			NodePool[result] = data;
			InternalMap.Add(data.Data, result);
			return result;
		}
	}
	else
	{
		NodePool[*findValue].RefCount++;
		return *findValue;
	}
}
NodeIndex FDAGNode::InsertData(const FNode& data)
{
	NodeIndex result;
	NodeIndex* findValue = DataMap.Find(data.Data);
	if (findValue == nullptr)
	{
		if (FreeIndex.IsEmpty())
		{
			result = NodePool.Num();
			NodePool.Emplace(data);
			DataMap.Add(data.Data, result);
			return result;
		}
		else
		{
			FreeIndex.Dequeue(result);
			NodePool[result] = data;
			DataMap.Add(data.Data, result);
			return result;
		}
	}
	else
	{
		NodePool[*findValue].RefCount++;
		return *findValue;
	}
}

void FDAGNode::SubtractInternalRef(NodeIndex index)
{
	if (index == INDEX_NONE)
	{
		return;
	}
	NodePool[index].RefCount--;
	if (NodePool[index].RefCount == 0)
	{
		FreeIndex.Enqueue(index);
		InternalMap.Remove(NodePool[index].Data);
	}
}

void FDAGNode::SubtractDataRef(NodeIndex index)
{
	if (index == INDEX_NONE)
	{
		return;
	}
	NodePool[index].RefCount--;
	if (NodePool[index].RefCount == 0)
	{
		FreeIndex.Enqueue(index);
		DataMap.Remove(NodePool[index].Data);
	}
}

NodeIndex FDAGNode::UpdataInternalChild(FNodeContext& context)
{
	if (context.NodeIndexDown == context.Node[context.ChildIndex])
		return context.Index;
	
	SubtractInternalRef(context.Index);

	context.Node[context.ChildIndex] = context.NodeIndexDown;
	context.Index = InsertInternal(context.Node);

	return context.Index;
}
NodeIndex FDAGNode::UpdataDataChild(FNodeContext& context)
{
	if (context.Data == context.Node[context.DataIndex])
		return context.Index;

	SubtractDataRef(context.Index);
	context.Node[context.DataIndex] = context.Data;
	context.Index = InsertData(context.Node);
	return context.Index;
}

const FNode& FDAGNode::GetNode(const NodeIndex& index)const
{
	return NodePool[index];
}

FVolume::FVolume()
{

}

void FVolume::SetVoxelData(const FIntVector& vector, uint32 data)
{
	FNodeContext context;
	context.Node = DAG.RootNode;
	context.Index = INDEX_NONE;
	context.Depth = MaxDepth;
	context.ChildIndexUp = NULL;
	context.ChildIndex = GetChildIndex(vector, context.Depth);
	context.NodeIndexUp = INDEX_NONE;
	context.NodeIndexDown = context.Node[context.ChildIndex];

	DAG.RootNode[context.ChildIndex] = RecursionSetVoxelData(vector, context, data);
}

NodeIndex FVolume::RecursionSetVoxelData(const FIntVector& vector,const FNodeContext& contextUp,const uint32& data)
{
	FNodeContext nowContext = contextUp.NodeDown();

	if (nowContext.Depth != 0)
	{
		if (nowContext.Index == INDEX_NONE)
		{
			nowContext.Node = FNode();
			nowContext.ChildIndex = GetChildIndex(vector, nowContext.Depth);
			nowContext.NodeIndexDown = RecursionSetVoxelData(vector, nowContext, data);
			nowContext.Node[nowContext.ChildIndex] = nowContext.NodeIndexDown;
			return DAG.InsertInternal(nowContext.Node);
		}
		else
		{
			nowContext.Node = DAG.GetNode(nowContext.Index);
			nowContext.ChildIndex = GetChildIndex(vector, nowContext.Depth);
			nowContext.NodeIndexDown = nowContext.Node[nowContext.ChildIndex];
			nowContext.NodeIndexDown = RecursionSetVoxelData(vector, nowContext, data);
			return DAG.UpdataInternalChild(nowContext);
		}
	}
	else if (nowContext.Depth == 0)
	{
		if (nowContext.Index == INDEX_NONE)
		{
			nowContext.Node = FNode();
			nowContext.DataIndex = GetChildIndex(vector, nowContext.Depth);
			nowContext.Data = data;
			nowContext.Node[nowContext.DataIndex] = nowContext.Data;
			return DAG.InsertData(nowContext.Node);
		}
		else
		{
			nowContext.Node = DAG.GetNode(nowContext.Index);
			nowContext.DataIndex = GetChildIndex(vector, nowContext.Depth);
			nowContext.Data = data;
			return DAG.UpdataDataChild(nowContext);
		}
	}
	check(false);
	return 0;
}

uint32 FVolume::GetVoxelData(const FIntVector& vector)const
{
	uint8 depth = MaxDepth;
	FNode nowNode = DAG.RootNode;
	NodeIndex nowNodeIndex = INDEX_NONE;

	while (true)
	{
		uint8 childNodeMask = GetChildIndex(vector, depth);
		NodeIndex nextNodeIndex = nowNode[childNodeMask];

		if (depth != 0)
		{
			if (nextNodeIndex == INDEX_NONE)
			{
				return INDEX_NONE;
			}
			else
			{
				nowNodeIndex = nextNodeIndex;
				nowNode = DAG.GetNode(nextNodeIndex);
			}
		}
		else
		{
			return nextNodeIndex;
		}
		depth--;
	}
}

PRAGMA_ENABLE_OPTIMIZATION