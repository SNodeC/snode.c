/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "apps/mqtt/server/TopicTree.h"

#include "apps/mqtt/server/SubscriberTree.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    apps::mqtt::server::TopicTree topicTree("", "");

    topicTree.publish("TopicA", "Value1");
    topicTree.publish("TopicA/Topic1", "Value11");
    topicTree.publish("TopicA/Topic2", "Value12");
    topicTree.publish("TopicB", "Value2");
    topicTree.publish("TopicB/Topic1", "Value21");
    topicTree.publish("TopicB/Topic2", "Value22");
    topicTree.publish("TopicB/Topic2/asdf/asf/adf/asdf/asdf/asdf/asdf", "ValueLooooooooooong");
    topicTree.publish("TopicB/Topic2/asdf/asf", "ValueLooooooooooong");
    topicTree.publish("TopicB/Topic2/asdf/asf", "ValueShortShoudOverwrite");

    topicTree.traverse();

    apps::mqtt::server::SubscriberTree subscriberTree;

    subscriberTree.subscribe("hihi/+/haha/+/hoho/#", nullptr);
    subscriberTree.subscribe("hihi/hui/haha/hoho/#", nullptr);
    subscriberTree.subscribe("hihi/hui/haha/huhu/hoho", nullptr);
    subscriberTree.subscribe("hihi/hui/haha/huhu/hoho", nullptr);

    subscriberTree.publish("hihi/hui/haha/huhu/hoho", "Nachricht");
    subscriberTree.publish("hihi/hui/haha/hoho/hoho/ja", "Nachricht1");
    subscriberTree.publish("hihi/hui/haha1/hoho/huri/ja", "Nachricht2");

    return 0;
}
