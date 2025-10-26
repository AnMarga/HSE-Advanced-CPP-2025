#pragma once
#include "dungeon.h"

#include <queue>
#include <string>
#include <unordered_set>

Room* FindFinalRoom(Room* start) {
    if (!start) {
        return nullptr;
    }

    std::unordered_set<Room*> visited;
    std::unordered_set<std::string> keys;
    std::queue<Room*> q;

    q.push(start);
    visited.insert(start);

    while (true) {
        bool new_key_found = false;
        std::queue<Room*> next_round;

        while (!q.empty()) {
            Room* room = q.front();
            q.pop();

            if (room->IsFinal()) {
                return room;
            }

            for (size_t i = 0; i < room->NumKeys(); ++i) {
                if (keys.insert(room->GetKey(i)).second) {
                    new_key_found = true;
                }
            }

            for (size_t i = 0; i < room->NumDoors(); ++i) {
                Door* door = room->GetDoor(i);

                if (!door->IsOpen()) {
                    for (const auto& key : keys) {
                        if (door->TryOpen(key)) {
                            break;
                        }
                    }
                }

                if (door->IsOpen()) {
                    Room* to = door->GoThrough();
                    if (!visited.count(to)) {
                        visited.insert(to);
                        next_round.push(to);
                    }
                }
            }
        }

        if (new_key_found) {
            for (Room* r : visited) {
                q.push(r);
            }
            continue;
        }

        if (!next_round.empty()) {
            q.swap(next_round);
            continue;
        }

        break;
    }

    return nullptr;
}
