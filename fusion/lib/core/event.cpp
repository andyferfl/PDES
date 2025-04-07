#include "core/event.hpp"

namespace fusion
{

Event::Event(double timestamp, uint64_t source_id, uint64_t target_id, uint32_t type_id)
    : timestamp_(timestamp),
        source_id_(source_id),
        target_id_(target_id),
        type_id_(type_id)
{
}

Event::Event(double timestamp, uint64_t source_id, uint64_t target_id, uint32_t type_id, std::any data)
: timestamp_(timestamp),
    source_id_(source_id),
    target_id_(target_id),
    type_id_(type_id),
    data_(data)
{
}

double Event::getTimestamp() const
{
    return timestamp_;
}

uint64_t Event::getSourceId() const
{
    return source_id_;
}

uint64_t Event::getTargetId() const
{
    return target_id_;
}

uint64_t Event::getTypeId() const
{
    return type_id_;
}

const std::any& Event::getData() const
{
    return data_;
}

void Event::setCreationTime(double creation_time)
{
    creation_time_ = creation_time;
}

double Event::getCreationTime() const
{
    return creation_time_;
}

void Event::setSendingLP(uint32_t lp_id)
{
    sending_lp_ = lp_id;
}

uint32_t Event::getSendingLP() const
{
    return sending_lp_;
}

void Event::setReceivingLP(uint32_t lp_id)
{
    receiving_lp_ = lp_id;
}

uint32_t Event::getReceivingLP() const
{
    return receiving_lp_;
}

void Event::setAntiMessage(bool is_anti_message)
{
    is_anti_message_ = is_anti_message;
}

bool Event::isAntiMessage() const
{
    return is_anti_message_;
}

}
