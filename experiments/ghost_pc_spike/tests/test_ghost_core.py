import unittest
from ghost_core import Ghost, HeartSnapshot, SemanticEvent

class GhostBoundaryTests(unittest.TestCase):
    def setUp(self):
        self.g = Ghost(HeartSnapshot(.6, .5, 0))

    def test_memory_relationship_does_not_duplicate_or_change_facts(self):
        self.g.record_event(SemanticEvent('e1', 'greeting', 100, 'person-A'), HeartSnapshot(.7, .6, 100))
        self.g.record_event(SemanticEvent('e2', 'greeting', 150, 'person-B'), HeartSnapshot(.8, .6, 150))
        a = self.g.relationship.evidence_for('person-A')
        self.assertEqual([e.event.event_id for e in a.events], ['e1'])
        self.assertEqual(self.g.memory.recall()[0].heart_before.mood, .6)
        self.assertEqual(self.g.memory.recall()[0].heart_after.mood, .7)
        self.assertEqual(len(self.g.memory.recall()), 2)

    def test_unknown_actor_must_not_be_attributed(self):
        self.g.record_event(SemanticEvent('e1', 'touch', 10), HeartSnapshot(.7, .5, 10))
        self.assertIsNone(self.g.relationship.evidence_for(None))
        self.assertEqual(self.g.relationship.evidence_for('person-A').events, ())

    def test_duplicate_rejected(self):
        event = SemanticEvent('same', 'touch', 10, 'a')
        self.g.record_event(event, HeartSnapshot(.7, .5, 10))
        with self.assertRaises(ValueError):
            self.g.record_event(event, HeartSnapshot(.9, .9, 12))
        self.assertEqual(self.g.heart.snapshot().mood, .7)

    def test_interpretation_does_not_erase_original_fear(self):
        self.g.record_event(SemanticEvent('e1', 'surprise', 10, 'a'), HeartSnapshot(.2, .1, 10))
        self.g.memory.reinterpret('e1', 'perhaps helped me')
        exp = self.g.memory.recall()[0]
        self.assertEqual(exp.heart_after.comfort, .1)
        self.assertEqual(exp.interpretation, 'perhaps helped me')

    def test_time_monotonic(self):
        self.g.time.tick(10)
        with self.assertRaises(ValueError):
            self.g.time.tick(9)

    def test_five_core_interfaces(self):
        self.assertEqual(set(vars(self.g)), {'heart', 'memory', 'time', 'relationship', 'behavior'})
        self.assertEqual(self.g.behavior.describe_inputs(self.g.heart.snapshot(), None)['evidence_count'], 0)

if __name__ == '__main__':
    unittest.main()
