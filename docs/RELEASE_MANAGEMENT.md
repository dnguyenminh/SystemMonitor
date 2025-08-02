# SystemMonitor Release Management Guide

This document describes the complete git flow and release process for SystemMonitor.

## 🚀 Quick Release Commands

```bash
# Complete release workflow
release.bat prepare v1.0.0    # Build and package release
release.bat push v1.0.0       # Push to GitHub

# Individual commands
release.bat status            # Check git status
release.bat build            # Build only
release.bat clean            # Clean artifacts
```

## 📋 Release Process

### Step 1: Prepare Release
```bash
release.bat prepare v1.0.0
```
This command:
- ✅ Builds SystemMonitor with static linking
- ✅ Creates release package in `releases/v1.0.0/`
- ✅ Generates deployment documentation
- ✅ Creates ZIP archive for distribution
- ✅ Commits changes and creates git tag

### Step 2: Push to GitHub
```bash
release.bat push v1.0.0
```
This command:
- ✅ Pushes main branch to origin
- ✅ Pushes version tag to origin
- ✅ Triggers GitHub Actions workflow (if configured)

### Step 3: Create GitHub Release
1. Go to GitHub repository → Releases
2. Click "Create a new release"
3. Select tag `v1.0.0`
4. Upload the generated ZIP file
5. Use auto-generated release notes or customize

## 🏗️ GitHub Actions Workflow

The included `.github/workflows/build-release.yml` provides:
- ✅ Automated building on tag push
- ✅ Dependency management with vcpkg
- ✅ Release package creation
- ✅ Automatic GitHub release creation

### Trigger Workflows:
```bash
git tag v1.0.0
git push origin v1.0.0    # Automatically triggers build
```

## 📦 Release Package Contents

Each release contains:
```
SystemMonitor-v1.0.0/
├── SystemMonitor.exe              # Main executable (static linked)
├── config/
│   └── SystemMonitor.cfg.template # Configuration template
├── docs/                          # Documentation
├── README.md                      # Project readme
└── DEPLOYMENT_README.md           # Deployment guide
```

## 🔧 Configuration Management

### For Development:
- Use `config/SystemMonitor.cfg` (ignored by git)
- Contains your actual email credentials

### For Releases:
- Use `config/SystemMonitor.cfg.template` (tracked by git)
- Contains example configuration without sensitive data
- Users copy and customize for their environment

## 🌟 Best Practices

### Version Numbering:
- Use semantic versioning: `v1.0.0`
- Major.Minor.Patch format
- Tag format: `vX.Y.Z`

### Commit Messages:
```bash
# Good commit messages
git commit -m "feat: Add TLS email notification support"
git commit -m "fix: Resolve mutex deadlock in system monitoring"
git commit -m "docs: Update deployment instructions"

# Release commits
git commit -m "Release v1.0.0: Production build with static linking"
```

### Branch Strategy:
- `main` - Stable, production-ready code
- `develop` - Integration branch for features
- `feature/*` - Individual feature branches
- `hotfix/*` - Critical fixes for releases

## 🔄 Complete Workflow Example

```bash
# 1. Prepare your code
git add .
git commit -m "feat: Complete SystemMonitor with TLS email alerts"

# 2. Create release
release.bat prepare v1.0.0

# 3. Push to GitHub
release.bat push v1.0.0

# 4. GitHub Actions automatically:
#    - Builds the project
#    - Creates release package
#    - Publishes GitHub release
```

## 🚨 Troubleshooting

### Build Fails:
```bash
release.bat clean    # Clean artifacts
release.bat build    # Test build
```

### Git Issues:
```bash
release.bat status   # Check repository state
git log --oneline    # Review recent commits
```

### Release Package Issues:
- Check `releases/` directory for generated files
- Verify ZIP archive contents
- Test deployment package on clean machine

## 📊 Release Checklist

Before creating a release:
- [ ] All tests pass
- [ ] Documentation updated
- [ ] Configuration template updated
- [ ] Version numbers updated
- [ ] CHANGELOG.md updated (if applicable)
- [ ] No sensitive data in repository

After creating a release:
- [ ] GitHub release published
- [ ] Release notes complete
- [ ] Deployment tested
- [ ] Documentation links working
- [ ] Archive downloadable

## 🔗 Related Files

- `release.bat` - Main release management script
- `scripts/prepare-release.bat` - Detailed release preparation
- `.github/workflows/build-release.yml` - CI/CD automation
- `config/SystemMonitor.cfg.template` - Configuration template
- `.gitignore` - Git ignore rules for releases
