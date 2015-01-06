/**
 *  @file ListEntry.h
 *
 *  @brief Declaration of ListEntry
 */
#pragma once

/**
 * @brief Simple LIST_ENTRY wrapper
 */
class ListEntry : public LIST_ENTRY
{
public:
	ListEntry()
	{
		Flink = Blink = this;
	}
	void Append(LIST_ENTRY *p)
	{
		p->Flink = Blink->Flink;
		p->Blink = Blink;
		Blink->Flink = p;
		Blink = p;
	}
	void RemoveSelf()
	{
		Blink->Flink = Flink;
		Flink->Blink = Blink;
		Flink = Blink = this;
	}
	ListEntry *IsSibling(LIST_ENTRY *p) const
	{
		return p != static_cast<const ListEntry *>(this) ?
					static_cast<ListEntry *>(p) : 0;
	}
	bool IsSolitary() const
	{
		return Flink == this;
	}
private:
	ListEntry(const ListEntry &); // disallow copy construction
	void operator=(const ListEntry &); // disallow assignment
};
