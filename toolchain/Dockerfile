FROM eduardorfs/reason-mobile:base

# RUN echo 'Server = http://linorg.usp.br/archlinux/$repo/os/$arch' > /etc/pacman.d/mirrorlist
RUN pacman -Sy --noconfirm unzip wget base base-devel zsh zsh-syntax-highlighting nodejs npm git

# root without zsh is sad
RUN chsh -s /usr/bin/zsh root; \
  sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"; \
  echo "source /usr/share/zsh/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh" >> /root/.zshrc

ARG TOOLCHAIN_USER

# you also needs zsh
RUN useradd -m ${TOOLCHAIN_USER}; \
  chsh -s /usr/bin/zsh ${TOOLCHAIN_USER}; \
  su ${TOOLCHAIN_USER} -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"; \
  echo "source /usr/share/zsh/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh" >> /home/${TOOLCHAIN_USER}/.zshrc;

RUN su ${TOOLCHAIN_USER} -c "curl -o- -L https://yarnpkg.com/install.sh | bash; ~/.yarn/bin/yarn global add esy"

# needs because of /usr/local/bin/platform
RUN echo "${TOOLCHAIN_USER} ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers; \
  echo "Set disable_coredump false" > /etc/sudo.conf;

# avoid cache invalidation
RUN ln -s /app/.esy /home/${TOOLCHAIN_USER}/.esy 
