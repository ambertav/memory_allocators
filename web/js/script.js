const config = {
  linear: {
    title: 'Linear Allocator',
    controls: ['allocate', 'resize_last', 'reset'],
  },
  'free-list': {
    title: 'Free List Allocator',
    controls: ['allocate', 'deallocate', 'reset'],
  },
  buddy: {
    title: 'Buddy Allocator',
    controls: ['allocate', 'deallocate', 'reset'],
  },
};

document.addEventListener('DOMContentLoaded', () => {
  let current_allocator = '';

  document.querySelectorAll('.toggle').forEach((button) => {
    button.addEventListener('click', () => {
      const type = button.dataset.type;
      current_allocator = type;

      const previous = document.querySelector('.selected');
      if (previous) previous.classList.remove('selected');
      button.classList.add('selected');

      renderInfo(type);
      renderControls(type);
    });
  });

  function renderInfo(type) {
    const info = config[type];
    document.querySelector('.title').textContent = info.title;
  }

  function renderControls(type) {
    const info = config[type];
    const controls = document.querySelector('.controls');

    let html = `<h4>Controls</h4>`;

    if (info.controls.includes('allocate')) {
      html += `<button id="allocate-button">Allocate</button>`;
    }
    if (info.controls.includes('deallocate')) {
      html += `<button id="deallocate-button">Deallocate</button>`;
    }
    if (info.controls.includes('resize_last')) {
      html += `<button id="resize-button">Resize Last</button>`;
    }
    if (info.controls.includes('reset')) {
      html += `<button id="reset-button">Reset</button>`;
    }

    controls.innerHTML = html;
    attachControlHandlers(type);
  }

  function attachControlHandlers(type) {
    const allocateButton = document.getElementById('allocate-button');
    const deallocateButton = document.getElementById('deallocate-button');
    const resizeButton = document.getElementById('resize-button');
    const resetButton = document.getElementById('reset-button');

    if (allocateButton) {
      allocateButton.onclick = () => {
        console.log('allocate');
      };
    }

    if (deallocateButton) {
      deallocateButton.onclick = () => {
        console.log('deallocate');
      };
    }

    if (resizeButton) {
      resizeButton.onclick = () => {
        console.log('resize_last');
      };
    }

    if (resetButton) {
      resetButton.onclick = () => {
        console.log('reset');
      };
    }
  }
});
