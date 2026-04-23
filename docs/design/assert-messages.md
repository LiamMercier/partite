# Assert Message Format

The following are suggestions for writing assert messages in this library with the goal of making error messages easier for users to make use of.

## Shape

Each assert should be formatted as:

ClassName::function_name(args): failure reason; expected values or context

This gives you:

- What failed (this operation)
- Where (calls to ClassName::function_name)
- Why (bad value, invariant violated, etc)
- Which values were bad, and what was expected
