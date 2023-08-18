/*!
* \file list.c
* \brief �������
* 
*��ϸ���� 
* 
* \author zbaofu
* \version V1.1
* \date 20230811 
*/

#include "list.h"

/* ����ڵ��ʼ�� */
void vListInitialiseItem(ListItem_t * const pxItem){
	// ��ʼ���ýڵ����ڵ�����Ϊ�գ���ʾ�ڵ�δ�����κ�����
	pxItem->pvContainer = NULL;
}

/* ������ڵ��ʼ�� */
void vListInitialise(List_t *const pxList){
	/* ����������ָ��ָ�����һ���ڵ� */
	pxList->pxIndex = (ListItem_t*) & (pxList->xListEnd);
	
	/* ���������һ���ڵ�ĸ��������ֵ����Ϊ���ȷ���ýڵ�����������ڵ� */
	pxList->xListEnd.xItemValue = portMAX_DELAY;
	
	/* �����һ���ڵ��pxNext��pxPreviousָ���ָ��ڵ�������ʾ����Ϊ�� */
	pxList->xListEnd.pxNext = (ListItem_t *) & (pxList->xListEnd);
	pxList->xListEnd.pxPrevious = (ListItem_t *) & (pxList->xListEnd);
	
	/* ��ʼ������ڵ��������ֵΪ0����ʾ����Ϊ�� */
	pxList->uxNumberOfItems = (UBaseType_t) 0U;

}

/* ���ڵ���뵽�����β�����ڵ� */
void vListInsertEnd(List_t * const pxList,ListItem_t *const pxNewListItem){
	// �����ڵ������ָ���¶���Ľڵ�
	ListItem_t *const pxIndex = pxList->pxIndex;
	
	pxNewListItem->pxNext = pxIndex;
	pxNewListItem->pxPrevious = pxIndex->pxPrevious;
	pxIndex->pxPrevious->pxNext = pxNewListItem;
	pxIndex->pxPrevious = pxNewListItem;
	
	/* ��ס�ڵ����ڵ����� */
	pxNewListItem->pvContainer = (void*) pxList;
	
	/* �����������һ */
	(pxList->uxNumberOfItems) ++;
	
}


/* ���ڵ㰴���������в��뵽���� */
void vListInsert(List_t *const pxList,ListItem_t *const pxNewListItem){
	ListItem_t *pxIterator;
	
	/*��ȡ�ڵ��������ֵ */
	const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;
	
	/* Ѱ�ҽڵ�Ҫ�����λ�� */
	if(xValueOfInsertion == portMAX_DELAY){
		pxIterator = pxList->xListEnd.pxPrevious;
	}
	else{
		for(pxIterator = (ListItem_t * )&(pxList->xListEnd); 
				pxIterator->pxNext->xItemValue<=xValueOfInsertion;
				pxIterator = pxIterator->pxNext
		){
			// ���ϱȽϽڵ㸨��ֵ���ҵ��ڵ�Ҫ�����λ��
		}
	
	}
	
	/* �������򣬸��������ҵ���λ�ò���ڵ� */
	pxNewListItem->pxNext = pxIterator->pxNext;
	pxNewListItem->pxNext->pxPrevious = pxNewListItem;
	pxNewListItem->pxPrevious = pxIterator;
	pxIterator->pxNext = pxNewListItem;
	
	/* ��ס�ýڵ����ڵ����� */
	pxNewListItem->pvContainer = (void*) pxList;
	
	/* �����������һ */
	(pxList->uxNumberOfItems) ++;
	
}

/* ���ڵ������ɾ�� */
UBaseType_t uxListRemove(ListItem_t * const pxItemRemove){
	/* ��ȡ�ڵ����ڵ����� */
	List_t * const pxList = (List_t *) pxItemRemove->pvContainer;
	
	/*��ָ���ڵ������ɾ�� */
	pxItemRemove->pxNext->pxPrevious = pxItemRemove->pxPrevious;
	pxItemRemove->pxPrevious->pxNext = pxItemRemove->pxNext;
	
	
	/* ��������ڵ�����ָ�� */
	if(pxList->pxIndex == pxItemRemove){
		pxList->pxIndex = pxItemRemove->pxPrevious;
	}
	
	/* ��ʼ���ýڵ���������Ϊ�գ���ʾ�ڵ㻹û�в����κ����� */
	pxItemRemove->pvContainer = NULL;
	
	/*����ڵ������-- */
	(pxList->uxNumberOfItems)--;
	
	/* ����������ʣ��ڵ����� */
	return pxList->uxNumberOfItems;

}

