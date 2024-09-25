#pragma once

namespace FinalProject {

/**
 * Duy: It is better to have a template struct Node which can store data of arbitrary type
 * E.g. Node<Student> where Student is a sortable class of the following:
 * - Matriculation ID
 * - Name
 * - Current semester
 * And in "class Student", there is a three-way comparator - you can check the slides or the following:
 *  https://en.cppreference.com/w/cpp/language/default_comparisons
 *
 * Consider this a bonus.
 * If you want to get this bonus, please send me an email containing all your modifications,
 *  including new tests/examples.
 * This should also include list.cc and list.h files
 */
struct Node {
  int key;
  int value;
  Node *next;

  Node(int key, int value, Node *next) : key(key), value(value), next(next) {}

  ~Node() = default;
};

}  // namespace FinalProject