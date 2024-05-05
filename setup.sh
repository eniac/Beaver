echo "Setting up the repository..."

HOOKS_DIR=${PWD}/.git/hooks
cp scripts/pre-commit $HOOKS_DIR/pre-commit
cp scripts/commit-msg $HOOKS_DIR/commit-msg

pip3 install -r requirements.txt

echo "Done."
