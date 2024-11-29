#pragma once

namespace FinalProject {
template <typename T>
struct Node{
  T value;
  Node *next;

  Node(T value, Node *next) : value(value), next(next){}
  ~Node() = default;
};

}  // namespace FinalProject
